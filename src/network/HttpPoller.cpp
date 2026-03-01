#include "network/HttpPoller.h"

#include "network/JsonParser.h"

#include <QNetworkRequest>

HttpPoller::HttpPoller(const QUrl &url, QObject *parent)
    : QObject(parent)
    , m_url(url)
{
}

void HttpPoller::start()
{
    // QNAM must be created on the thread this object lives on — called from
    // networkThread->started signal after moveToThread().
    m_nam = new QNetworkAccessManager(this);

    m_pollTimer = new QTimer(this);
    m_pollTimer->setInterval(kPollIntervalMs);
    connect(m_pollTimer, &QTimer::timeout, this, &HttpPoller::poll);

    // Fire the first poll immediately, then let the timer drive subsequent ones.
    poll();
    m_pollTimer->start();
}

void HttpPoller::poll()
{
    // Abort any in-flight reply before issuing a new one.
    if (m_pendingReply) {
        m_pendingReply->abort();
        m_pendingReply->deleteLater();
        m_pendingReply = nullptr;
    }

    QNetworkRequest req(m_url);
    req.setTransferTimeout(5000);

    m_pendingReply = m_nam->get(req);
    connect(m_pendingReply, &QNetworkReply::finished, this, &HttpPoller::onReply);
}

void HttpPoller::onReply()
{
    auto *reply = qobject_cast<QNetworkReply *>(sender());

    if (reply->error() == QNetworkReply::NoError) {
        const auto parsed = JsonParser::parseCurrentConditions(reply->readAll());
        if (parsed.iss) {
            emit issReceived(*parsed.iss);
        }
        if (parsed.bar) {
            emit barReceived(*parsed.bar);
        }
        if (parsed.indoor) {
            emit indoorReceived(*parsed.indoor);
        }
    }
    // On error: silently ignore — the next 10s poll is the retry (DATA-09).

    // MANDATORY: always release the reply regardless of success or failure.
    reply->deleteLater();
    m_pendingReply = nullptr;
}
