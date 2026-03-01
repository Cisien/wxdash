#pragma once

#include "models/WeatherReadings.h"

#include <QElapsedTimer>
#include <QObject>
#include <QTimer>
#include <functional>

/**
 * WeatherDataModel — single source of truth for all current weather values.
 *
 * Each field is exposed via a Q_PROPERTY with a NOTIFY signal.
 * Staleness is detected when no update is received for 30 seconds:
 *   - sourceStaleChanged(true) is emitted
 *   - all values are cleared to zero (no last-known display)
 * Recovery is silent: when an update arrives after staleness,
 *   sourceStaleChanged(false) is emitted.
 *
 * An injectable elapsed-time provider allows unit tests to control
 * the fake clock without 30-second real waits.
 */
class WeatherDataModel : public QObject {
    Q_OBJECT

    Q_PROPERTY(double temperature READ temperature NOTIFY temperatureChanged)
    Q_PROPERTY(double humidity READ humidity NOTIFY humidityChanged)
    Q_PROPERTY(double dewPoint READ dewPoint NOTIFY dewPointChanged)
    Q_PROPERTY(double heatIndex READ heatIndex NOTIFY heatIndexChanged)
    Q_PROPERTY(double windChill READ windChill NOTIFY windChillChanged)
    Q_PROPERTY(double windSpeed READ windSpeed NOTIFY windSpeedChanged)
    Q_PROPERTY(int windDir READ windDir NOTIFY windDirChanged)
    Q_PROPERTY(double windGust READ windGust NOTIFY windGustChanged)
    Q_PROPERTY(double rainRate READ rainRate NOTIFY rainRateChanged)
    Q_PROPERTY(double rainfallDaily READ rainfallDaily NOTIFY rainfallDailyChanged)
    Q_PROPERTY(double pressure READ pressure NOTIFY pressureChanged)
    Q_PROPERTY(int pressureTrend READ pressureTrend NOTIFY pressureTrendChanged)
    Q_PROPERTY(double uvIndex READ uvIndex NOTIFY uvIndexChanged)
    Q_PROPERTY(double solarRad READ solarRad NOTIFY solarRadChanged)
    Q_PROPERTY(double tempIn READ tempIn NOTIFY tempInChanged)
    Q_PROPERTY(double humIn READ humIn NOTIFY humInChanged)
    Q_PROPERTY(double dewPointIn READ dewPointIn NOTIFY dewPointInChanged)
    Q_PROPERTY(bool sourceStale READ sourceStale NOTIFY sourceStaleChanged)

public:
    explicit WeatherDataModel(QObject* parent = nullptr,
                              std::function<qint64()> elapsedProvider = {});

    // READ accessors
    double temperature() const { return m_temperature; }
    double humidity() const { return m_humidity; }
    double dewPoint() const { return m_dewPoint; }
    double heatIndex() const { return m_heatIndex; }
    double windChill() const { return m_windChill; }
    double windSpeed() const { return m_windSpeed; }
    int windDir() const { return m_windDir; }
    double windGust() const { return m_windGust; }
    double rainRate() const { return m_rainRate; }
    double rainfallDaily() const { return m_rainfallDaily; }
    double pressure() const { return m_pressure; }
    int pressureTrend() const { return m_pressureTrend; }
    double uvIndex() const { return m_uvIndex; }
    double solarRad() const { return m_solarRad; }
    double tempIn() const { return m_tempIn; }
    double humIn() const { return m_humIn; }
    double dewPointIn() const { return m_dewPointIn; }
    bool sourceStale() const { return m_sourceStale; }

public slots:
    void applyIssUpdate(const IssReading& r);
    void applyBarUpdate(const BarReading& r);
    void applyIndoorUpdate(const IndoorReading& r);
    void applyUdpUpdate(const UdpReading& r);
    void checkStaleness();

signals:
    void temperatureChanged(double value);
    void humidityChanged(double value);
    void dewPointChanged(double value);
    void heatIndexChanged(double value);
    void windChillChanged(double value);
    void windSpeedChanged(double value);
    void windDirChanged(int value);
    void windGustChanged(double value);
    void rainRateChanged(double value);
    void rainfallDailyChanged(double value);
    void pressureChanged(double value);
    void pressureTrendChanged(int value);
    void uvIndexChanged(double value);
    void solarRadChanged(double value);
    void tempInChanged(double value);
    void humInChanged(double value);
    void dewPointInChanged(double value);
    void sourceStaleChanged(bool stale);

private:
    void clearAllValues();
    void markUpdated();

    // Weather fields
    double m_temperature = 0.0;
    double m_humidity = 0.0;
    double m_dewPoint = 0.0;
    double m_heatIndex = 0.0;
    double m_windChill = 0.0;
    double m_windSpeed = 0.0;
    int m_windDir = 0;
    double m_windGust = 0.0;
    double m_rainRate = 0.0;
    double m_rainfallDaily = 0.0;
    double m_pressure = 0.0;
    int m_pressureTrend = 0;
    double m_uvIndex = 0.0;
    double m_solarRad = 0.0;
    double m_tempIn = 0.0;
    double m_humIn = 0.0;
    double m_dewPointIn = 0.0;
    bool m_sourceStale = false;

    // Staleness tracking
    static constexpr int kStalenessMs = 30000;
    bool m_hasReceivedUpdate = false;
    qint64 m_lastUpdateElapsed = 0; // Value from elapsed provider at time of last update
    QTimer* m_stalenessTimer = nullptr;
    std::function<qint64()> m_elapsedProvider;
    QElapsedTimer m_wallClock; // Used when no provider is injected
};
