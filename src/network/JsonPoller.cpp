#include "network/JsonPoller.h"

#include <QNetworkRequest>

JsonPoller::JsonPoller(const QUrl &url, int pollIntervalMs, QObject *parent)
    : QObject(parent)
    , m_url(url)
    , m_pollIntervalMs(pollIntervalMs)
{
}

void JsonPoller::start()
{
    m_nam = new QNetworkAccessManager(this);

    m_pollTimer = new QTimer(this);
    m_pollTimer->setInterval(m_pollIntervalMs);
    connect(m_pollTimer, &QTimer::timeout, this, &JsonPoller::poll);

    poll();
    m_pollTimer->start();
}

void JsonPoller::poll()
{
    if (m_pendingReply) {
        m_pendingReply->abort();
        m_pendingReply->deleteLater();
        m_pendingReply = nullptr;
    }

    QNetworkRequest req(m_url);
    req.setTransferTimeout(transferTimeoutMs());
    configureRequest(req);

    m_pendingReply = m_nam->get(req);
    connect(m_pendingReply, &QNetworkReply::finished, this, &JsonPoller::onReply);
}

void JsonPoller::onReply()
{
    auto *reply = m_pendingReply;
    m_pendingReply = nullptr;

    if (!reply) return;
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) return;
    if (!validateReply(reply)) return;

    handleResponse(reply->readAll());
}
