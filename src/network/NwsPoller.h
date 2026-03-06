#pragma once

#include "models/WeatherReadings.h"
#include "network/JsonPoller.h"

#include <QVector>

/**
 * NwsPoller — polls NWS forecast API every 30 minutes and emits
 * parsed ForecastDay data.
 *
 * Must be moved to a QThread before start() is called.
 * On API failure, silently keeps last forecast (no staleness clearing).
 */
class NwsPoller : public JsonPoller {
    Q_OBJECT

public:
    explicit NwsPoller(const QUrl &url, QObject *parent = nullptr);

signals:
    void forecastReceived(QVector<ForecastDay> forecast);

protected:
    int transferTimeoutMs() const override { return 10000; }
    void configureRequest(QNetworkRequest &req) override;
    bool validateReply(QNetworkReply *reply) override;
    void handleResponse(const QByteArray &data) override;
};
