#include "network/NwsPoller.h"
#include "network/JsonParser.h"

NwsPoller::NwsPoller(const QUrl &url, QObject *parent)
    : QObject(parent), m_url(url) {}

void NwsPoller::start() {
    m_nam = new QNetworkAccessManager(this);
    m_pollTimer = new QTimer(this);
    m_pollTimer->setInterval(kPollIntervalMs);
    connect(m_pollTimer, &QTimer::timeout, this, &NwsPoller::poll);
    m_pollTimer->start();
    poll(); // immediate first poll on startup
}

void NwsPoller::poll() {
    // Abort any in-flight request
    if (m_pendingReply) {
        m_pendingReply->abort();
        m_pendingReply->deleteLater();
        m_pendingReply = nullptr;
    }

    QNetworkRequest req(m_url);
    req.setTransferTimeout(10000); // 10s — NWS can be slow (external API vs local)
    // NWS API REQUIRES User-Agent header or returns HTTP 403
    req.setRawHeader("User-Agent", "wxdash/1.0 (github.com/cisien/wxdash)");

    m_pendingReply = m_nam->get(req);
    connect(m_pendingReply, &QNetworkReply::finished, this, &NwsPoller::onReply);
}

void NwsPoller::onReply() {
    auto *reply = m_pendingReply;
    m_pendingReply = nullptr;

    if (!reply) return;
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) return;

    // Check HTTP status — NWS returns 403 (not a QNetworkReply error) without User-Agent
    int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (httpStatus != 200) return;

    auto forecast = JsonParser::parseForecast(reply->readAll());
    if (!forecast.isEmpty()) {
        emit forecastReceived(forecast);
    }
    // On error or empty: silently keep last forecast per user decision
}
