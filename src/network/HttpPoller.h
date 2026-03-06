#pragma once

#include "models/WeatherReadings.h"
#include "network/JsonPoller.h"

/**
 * HttpPoller — polls /v1/current_conditions every 10 seconds and emits
 * typed signals for each sensor type found in the response.
 *
 * Must be moved to a dedicated QThread before start() is called.
 */
class HttpPoller : public JsonPoller {
    Q_OBJECT

public:
    explicit HttpPoller(const QUrl &url, QObject *parent = nullptr);

signals:
    void issReceived(IssReading reading);
    void barReceived(BarReading reading);
    void indoorReceived(IndoorReading reading);

protected:
    void handleResponse(const QByteArray &data) override;
};
