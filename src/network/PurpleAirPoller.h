#pragma once

#include "models/WeatherReadings.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QTimer>
#include <QUrl>

/**
 * PurpleAirPoller — polls PurpleAir local sensor API every 30 seconds
 * and emits a PurpleAirReading signal with averaged PM2.5 and AQI.
 *
 * Must be moved to a dedicated QThread before start() is called.
 * QNAM and poll timer are created inside start() so they are owned by
 * the correct thread.
 *
 * Per Claude's discretion: 30s interval. PurpleAir hardware averages
 * over 2-minute windows; polling faster provides no additional data.
 */
class PurpleAirPoller : public QObject {
    Q_OBJECT

public:
    static constexpr int kPollIntervalMs = 30000; // 30s between polls

    explicit PurpleAirPoller(const QUrl &url, QObject *parent = nullptr);

public slots:
    void start();

signals:
    void purpleAirReceived(PurpleAirReading reading);

private slots:
    void poll();
    void onReply();

private:
    QUrl m_url;
    QNetworkAccessManager *m_nam = nullptr;
    QTimer *m_pollTimer = nullptr;
    QNetworkReply *m_pendingReply = nullptr;
};
