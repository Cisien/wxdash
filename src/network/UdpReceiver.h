#pragma once

#include "models/WeatherReadings.h"

#include <QElapsedTimer>
#include <QNetworkAccessManager>
#include <QObject>
#include <QTimer>
#include <QUdpSocket>
#include <QUrl>

/**
 * UdpReceiver — receives real-time UDP broadcast datagrams from the
 * WeatherLink Live device on port 22222 and emits a typed signal per packet.
 *
 * Session management:
 *   - Initial broadcast is started by issuing GET /v1/real_time?duration=86400.
 *   - A health-check timer fires every 5s. If no packet has been received in
 *     the last 10s the broadcast is re-registered immediately (handles
 *     WeatherLink Live reboots that lose broadcast state).
 *   - The 86400s duration covers a full 24h window; no periodic renewal needed.
 *
 * Own QNAM:
 *   UdpReceiver creates its own QNetworkAccessManager in start() for the
 *   fire-and-forget broadcast HTTP requests. This is simpler than sharing
 *   HttpPoller's QNAM across a signal/slot boundary.
 *
 * Per locked decision (DATA-09): silent recovery — while UDP is silent the
 * health timer triggers re-registration; HTTP polling continues in parallel.
 */
class UdpReceiver : public QObject {
    Q_OBJECT

public:
    static constexpr int kHealthCheckMs = 5000;             // 5s health poll
    static constexpr int kSilenceThresholdMs = 10000;       // 10s silence → re-register
    static constexpr int kReregistrationCooldownMs = 60000; // 60s between re-registrations
    static constexpr int kNamResetThreshold = 5;            // 5 min silence → recycle QNAM

    explicit UdpReceiver(const QUrl &realtimeUrl, QObject *parent = nullptr);

    void setVerbose(bool v) { m_verbose = v; }

public slots:
    /** Bind UDP socket, create QNAM, start timers, issue initial broadcast. */
    void start();

signals:
    void realtimeReceived(UdpReading reading);

private slots:
    void onReadyRead();
    void renewBroadcast();
    void checkUdpHealth();

private:
    void resetNam();

    QUrl m_realtimeUrl;
    QUdpSocket *m_socket = nullptr;
    QNetworkAccessManager *m_nam = nullptr;
    QTimer *m_healthTimer = nullptr;
    QElapsedTimer m_lastPacketTimer;
    QElapsedTimer m_lastRegistrationTimer;
    int m_consecutiveReregistrations = 0;
    bool m_verbose = false;
};
