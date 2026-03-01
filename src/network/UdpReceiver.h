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
 *   - A renewal timer fires every 3600s (1h) to re-issue the start request,
 *     even though we requested 86400s — belt and suspenders (DATA-03).
 *   - A health-check timer fires every 5s. If no packet has been received in
 *     the last 10s the broadcast is re-registered immediately (handles
 *     WeatherLink Live reboots that lose broadcast state).
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
    static constexpr int kRenewalIntervalMs = 3600 * 1000; // 1h
    static constexpr int kHealthCheckMs = 5000;             // 5s health poll
    static constexpr int kSilenceThresholdMs = 10000;       // 10s silence → re-register

    explicit UdpReceiver(const QUrl &realtimeUrl, QObject *parent = nullptr);

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
    QUrl m_realtimeUrl;
    QUdpSocket *m_socket = nullptr;
    QNetworkAccessManager *m_nam = nullptr;
    QTimer *m_renewalTimer = nullptr;
    QTimer *m_healthTimer = nullptr;
    QElapsedTimer m_lastPacketTimer;
};
