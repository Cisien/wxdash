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

    // Delay the first poll to allow mDNS and the network stack to settle
    // after boot, then let the repeating timer drive subsequent polls.
    QTimer::singleShot(2000, this, &JsonPoller::poll);
    m_pollTimer->start();
}

void JsonPoller::poll()
{
    // Disconnect and discard any in-flight reply.
    // Must disconnect before abort() because abort() emits finished()
    // synchronously, which would re-enter onReply() and null m_pendingReply.
    if (m_pendingReply) {
        disconnect(m_pendingReply, nullptr, this, nullptr);
        m_pendingReply->abort();
        m_pendingReply->deleteLater();
        m_pendingReply = nullptr;
    }

    QNetworkRequest req(m_url);
    req.setTransferTimeout(transferTimeoutMs());
    configureRequest(req);

    if (m_verbose)
        qDebug("JsonPoller(%s): GET %s", qPrintable(m_url.host()),
               qPrintable(m_url.toString()));

    m_pendingReply = m_nam->get(req);
    connect(m_pendingReply, &QNetworkReply::finished, this, &JsonPoller::onReply);
}

void JsonPoller::onReply()
{
    auto *reply = m_pendingReply;
    m_pendingReply = nullptr;

    if (!reply) return;
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        ++m_consecutiveErrors;
        qWarning("JsonPoller(%s): error %d — %s (consecutive: %d)",
                 qPrintable(m_url.host()), reply->error(),
                 qPrintable(reply->errorString()), m_consecutiveErrors);
        if (m_consecutiveErrors >= kMaxConsecutiveErrors)
            resetNam();
        return;
    }
    if (!validateReply(reply)) return;

    m_consecutiveErrors = 0;
    const QByteArray data = reply->readAll();
    if (m_verbose)
        qDebug("JsonPoller(%s): %d bytes\n%s", qPrintable(m_url.host()),
               data.size(), data.constData());
    handleResponse(data);
}

void JsonPoller::resetNam()
{
    m_consecutiveErrors = 0;
    m_nam->deleteLater();
    m_nam = new QNetworkAccessManager(this);
}
