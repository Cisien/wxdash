#pragma once

#include "models/WeatherReadings.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QTimer>
#include <QUrl>
#include <QVector>

/**
 * NwsPoller — polls NWS forecast API every 30 minutes and emits
 * parsed ForecastDay data.
 *
 * Must be moved to a QThread before start() is called.
 * QNAM and timer created inside start() for correct thread affinity.
 *
 * Per user decision: 30-minute poll interval. NWS updates every few hours;
 * faster polling provides no benefit and risks IP blocks.
 * On API failure, silently keeps last forecast (no staleness clearing).
 */
class NwsPoller : public QObject {
    Q_OBJECT

public:
    static constexpr int kPollIntervalMs = 30 * 60 * 1000; // 30 minutes

    explicit NwsPoller(const QUrl &url, QObject *parent = nullptr);

public slots:
    void start();

signals:
    void forecastReceived(QVector<ForecastDay> forecast);

private slots:
    void poll();
    void onReply();

private:
    QUrl m_url;
    QNetworkAccessManager *m_nam = nullptr;
    QTimer *m_pollTimer = nullptr;
    QNetworkReply *m_pendingReply = nullptr;
};
