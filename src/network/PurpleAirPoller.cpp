#include "network/PurpleAirPoller.h"
#include "network/JsonParser.h"

PurpleAirPoller::PurpleAirPoller(const QUrl &url, QObject *parent)
    : QObject(parent), m_url(url) {}

void PurpleAirPoller::start() {
    m_nam = new QNetworkAccessManager(this);
    m_pollTimer = new QTimer(this);
    m_pollTimer->setInterval(kPollIntervalMs);
    connect(m_pollTimer, &QTimer::timeout, this, &PurpleAirPoller::poll);
    m_pollTimer->start();
    poll(); // immediate first poll
}

void PurpleAirPoller::poll() {
    // Abort any in-flight request
    if (m_pendingReply) {
        m_pendingReply->abort();
        m_pendingReply->deleteLater();
        m_pendingReply = nullptr;
    }

    QNetworkRequest req(m_url);
    req.setTransferTimeout(5000); // 5s timeout
    m_pendingReply = m_nam->get(req);
    connect(m_pendingReply, &QNetworkReply::finished, this, &PurpleAirPoller::onReply);
}

void PurpleAirPoller::onReply() {
    auto *reply = m_pendingReply;
    m_pendingReply = nullptr;

    if (!reply) return;
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) return;

    const QByteArray data = reply->readAll();
    auto reading = JsonParser::parsePurpleAirJson(data);

    // Only emit if we got valid data (non-zero AQI means parsing succeeded)
    // Note: AQI of 0 is valid (PM2.5 = 0), so always emit if parsing didn't fail
    emit purpleAirReceived(reading);
}
