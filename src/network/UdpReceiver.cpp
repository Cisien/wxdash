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
    if (!m_socket->bind(QHostAddress::AnyIPv4, 22222, QUdpSocket::ShareAddress))
        qWarning("UdpReceiver: failed to bind port 22222: %s",
                 qPrintable(m_socket->errorString()));
    connect(m_socket, &QUdpSocket::readyRead, this, &UdpReceiver::onReadyRead);

    // Health-check timer: detect silence longer than 10s and re-register.
    m_healthTimer = new QTimer(this);
    m_healthTimer->setInterval(kHealthCheckMs);
    connect(m_healthTimer, &QTimer::timeout, this, &UdpReceiver::checkUdpHealth);

    // Start the elapsed timer so checkUdpHealth() can detect silence
    // even if the first broadcast registration fails and no packet ever arrives.
    m_lastPacketTimer.start();

    // Start the first broadcast session immediately, then let timers drive renewal.
    renewBroadcast();
    m_healthTimer->start();
}

void UdpReceiver::onReadyRead()
{
    while (m_socket->hasPendingDatagrams()) {
        const QNetworkDatagram datagram = m_socket->receiveDatagram();
        const QByteArray raw = datagram.data();
        if (m_verbose)
            qDebug("UdpReceiver: %d bytes from %s:%d\n%s",
                   raw.size(),
                   qPrintable(datagram.senderAddress().toString()),
                   datagram.senderPort(),
                   raw.constData());
        const auto result = JsonParser::parseUdpDatagram(raw);
        if (result.has_value()) {
            m_lastPacketTimer.restart();
            m_consecutiveReregistrations = 0;
            emit realtimeReceived(*result);
        } else {
            qWarning("UdpReceiver: unparseable %d-byte packet from %s:%d\n%s",
                     raw.size(),
                     qPrintable(datagram.senderAddress().toString()),
                     datagram.senderPort(),
                     raw.constData());
        }
    }
}

void UdpReceiver::renewBroadcast()
{
    // Fire-and-forget GET to /v1/real_time?duration=86400.
    // We don't inspect the response body — a 200 means the session is active.
    QNetworkRequest req(m_realtimeUrl);
    req.setTransferTimeout(5000);
    if (m_verbose)
        qDebug("UdpReceiver: GET %s", qPrintable(m_realtimeUrl.toString()));

    QNetworkReply *reply = m_nam->get(req);
    // Log the registration response when verbose.
    if (m_verbose) {
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            if (reply->error() == QNetworkReply::NoError) {
                const QByteArray body = reply->readAll();
                qDebug("UdpReceiver: registration OK — %d bytes\n%s",
                       body.size(), body.constData());
            } else {
                qDebug("UdpReceiver: registration failed — %s",
                       qPrintable(reply->errorString()));
            }
            reply->deleteLater();
        });
    } else {
        connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    }

    m_lastRegistrationTimer.restart();
}

void UdpReceiver::resetNam()
{
    m_nam->deleteLater();
    m_nam = new QNetworkAccessManager(this);
}

void UdpReceiver::checkUdpHealth()
{
    if (!m_lastPacketTimer.isValid()
        || m_lastPacketTimer.elapsed() <= kSilenceThresholdMs)
        return; // packets flowing normally

    // Throttle re-registrations to once per cooldown period.
    if (m_lastRegistrationTimer.isValid()
        && m_lastRegistrationTimer.elapsed() < kReregistrationCooldownMs)
        return;

    ++m_consecutiveReregistrations;
    if (m_consecutiveReregistrations >= kNamResetThreshold) {
        qWarning("UdpReceiver: %d re-registrations without response, recycling QNAM",
                 m_consecutiveReregistrations);
        m_consecutiveReregistrations = 0;
        resetNam();
    }
    renewBroadcast();
}
