#pragma once

#include "models/WeatherReadings.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QTimer>
#include <QUrl>

/**
 * HttpPoller — polls /v1/current_conditions every 10 seconds and emits
 * typed signals for each sensor type found in the response.
 *
 * Must be moved to a dedicated QThread before start() is called.
 * QNAM and poll timer are created inside start() so they are owned by
 * the correct thread.
 *
 * Reply management:
 *   - If a reply is still in-flight when the timer fires, abort it first.
 *   - Transfer timeout: 5000ms per request.
 *   - Always call deleteLater() on the reply — no exceptions.
 *
 * Per locked decision (DATA-09): no special retry logic.
 * The 10s poll cadence IS the retry mechanism.
 */
class HttpPoller : public QObject {
    Q_OBJECT

public:
    static constexpr int kPollIntervalMs = 10000; // 10s between polls

    explicit HttpPoller(const QUrl &url, QObject *parent = nullptr);

public slots:
    /** Create QNAM and poll timer, then begin polling immediately. */
    void start();

signals:
    void issReceived(IssReading reading);
    void barReceived(BarReading reading);
    void indoorReceived(IndoorReading reading);

private slots:
    void poll();
    void onReply();

private:
    QUrl m_url;
    QNetworkAccessManager *m_nam = nullptr;
    QTimer *m_pollTimer = nullptr;
    QNetworkReply *m_pendingReply = nullptr;
};
