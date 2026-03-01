#include "network/UdpReceiver.h"

#include "network/JsonParser.h"

#include <QNetworkDatagram>
#include <QNetworkReply>
#include <QNetworkRequest>

UdpReceiver::UdpReceiver(const QUrl &realtimeUrl, QObject *parent)
    : QObject(parent)
    , m_realtimeUrl(realtimeUrl)
{
}

void UdpReceiver::start()
{
    // Create own QNAM for fire-and-forget broadcast HTTP requests.
    // Must be constructed on the thread this object lives on (after moveToThread).
    m_nam = new QNetworkAccessManager(this);

    // Bind to port 22222 shared with other listeners on the same machine.
    m_socket = new QUdpSocket(this);
    m_socket->bind(QHostAddress::AnyIPv4, 22222, QUdpSocket::ShareAddress);
    connect(m_socket, &QUdpSocket::readyRead, this, &UdpReceiver::onReadyRead);

    // Renewal timer: re-issue broadcast start every 1h (belt-and-suspenders).
    m_renewalTimer = new QTimer(this);
    m_renewalTimer->setInterval(kRenewalIntervalMs);
    connect(m_renewalTimer, &QTimer::timeout, this, &UdpReceiver::renewBroadcast);

    // Health-check timer: detect silence longer than 10s and re-register.
    m_healthTimer = new QTimer(this);
    m_healthTimer->setInterval(kHealthCheckMs);
    connect(m_healthTimer, &QTimer::timeout, this, &UdpReceiver::checkUdpHealth);

    // Start the first broadcast session immediately, then let timers drive renewal.
    renewBroadcast();
    m_renewalTimer->start();
    m_healthTimer->start();
}

void UdpReceiver::onReadyRead()
{
    while (m_socket->hasPendingDatagrams()) {
        const QNetworkDatagram datagram = m_socket->receiveDatagram();
        const auto result = JsonParser::parseUdpDatagram(datagram.data());
        if (result.has_value()) {
            m_lastPacketTimer.restart();
            emit realtimeReceived(*result);
        }
        // Silently ignore malformed packets — per locked decision (DATA-09).
    }
}

void UdpReceiver::renewBroadcast()
{
    // Fire-and-forget GET to /v1/real_time?duration=86400.
    // We don't inspect the response body — a 200 means the session is active.
    QNetworkRequest req(m_realtimeUrl);
    req.setTransferTimeout(5000);
    QNetworkReply *reply = m_nam->get(req);
    // Release reply memory as soon as the response arrives (no body needed).
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
}

void UdpReceiver::checkUdpHealth()
{
    if (m_lastPacketTimer.isValid()
        && m_lastPacketTimer.elapsed() > kSilenceThresholdMs) {
        // No packet received within the threshold — re-register broadcast session.
        renewBroadcast();
    }
}
