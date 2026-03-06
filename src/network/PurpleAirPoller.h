#pragma once

#include "models/WeatherReadings.h"
#include "network/JsonPoller.h"

/**
 * PurpleAirPoller — polls PurpleAir local sensor API every 30 seconds
 * and emits a PurpleAirReading signal with averaged PM2.5 and AQI.
 *
 * Must be moved to a dedicated QThread before start() is called.
 */
class PurpleAirPoller : public JsonPoller {
    Q_OBJECT

public:
    explicit PurpleAirPoller(const QUrl &url, QObject *parent = nullptr);

signals:
    void purpleAirReceived(PurpleAirReading reading);

protected:
    void handleResponse(const QByteArray &data) override;
};
