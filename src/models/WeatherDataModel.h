#pragma once

#include "models/WeatherReadings.h"

#include <QDataStream>
#include <QElapsedTimer>
#include <QObject>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>
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
    Q_PROPERTY(QVariantList windRoseData READ windRoseData NOTIFY windRoseDataChanged)
    Q_PROPERTY(int windRoseMaxCount READ windRoseMaxCount NOTIFY windRoseDataChanged)

    Q_PROPERTY(QVariantList temperatureHistory READ temperatureHistory NOTIFY temperatureHistoryChanged)
    Q_PROPERTY(QVariantList feelsLikeHistory READ feelsLikeHistory NOTIFY feelsLikeHistoryChanged)
    Q_PROPERTY(QVariantList humidityHistory READ humidityHistory NOTIFY humidityHistoryChanged)
    Q_PROPERTY(QVariantList dewPointHistory READ dewPointHistory NOTIFY dewPointHistoryChanged)
    Q_PROPERTY(QVariantList windSpeedHistory READ windSpeedHistory NOTIFY windSpeedHistoryChanged)
    Q_PROPERTY(QVariantList rainRateHistory READ rainRateHistory NOTIFY rainRateHistoryChanged)
    Q_PROPERTY(QVariantList pressureHistory READ pressureHistory NOTIFY pressureHistoryChanged)
    Q_PROPERTY(QVariantList uvIndexHistory READ uvIndexHistory NOTIFY uvIndexHistoryChanged)
    Q_PROPERTY(QVariantList solarRadHistory READ solarRadHistory NOTIFY solarRadHistoryChanged)

    Q_PROPERTY(double aqi READ aqi NOTIFY aqiChanged)
    Q_PROPERTY(double pm25 READ pm25 NOTIFY pm25Changed)
    Q_PROPERTY(double pm10 READ pm10 NOTIFY pm10Changed)
    Q_PROPERTY(bool purpleAirStale READ purpleAirStale NOTIFY purpleAirStaleChanged)
    Q_PROPERTY(QVariantList aqiHistory READ aqiHistory NOTIFY aqiHistoryChanged)

    Q_PROPERTY(QVariantList forecastData READ forecastData NOTIFY forecastDataChanged)

public:
    // Sparkline capacity: 24h at 10s cadence
    static constexpr int kSparklineCapacity = 8640;

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
    QVariantList windRoseData() const;
    int windRoseMaxCount() const;

    // Sparkline history accessors (chronological, oldest first)
    QVariantList temperatureHistory() const;
    QVariantList feelsLikeHistory() const;
    QVariantList humidityHistory() const;
    QVariantList dewPointHistory() const;
    QVariantList windSpeedHistory() const;
    QVariantList rainRateHistory() const;
    QVariantList pressureHistory() const;
    QVariantList uvIndexHistory() const;
    QVariantList solarRadHistory() const;

    // PurpleAir accessors
    double aqi() const { return m_aqi; }
    double pm25() const { return m_pm25; }
    double pm10() const { return m_pm10; }
    bool purpleAirStale() const { return m_purpleAirStale; }
    QVariantList aqiHistory() const;

    // Forecast accessor
    QVariantList forecastData() const;

    void saveSparklineData(const QString& path) const;
    void loadSparklineData(const QString& path);

public slots:
    void applyIssUpdate(const IssReading& r);
    void applyBarUpdate(const BarReading& r);
    void applyIndoorUpdate(const IndoorReading& r);
    void applyUdpUpdate(const UdpReading& r);
    void applyPurpleAirUpdate(const PurpleAirReading& r);
    void applyForecastUpdate(const QVector<ForecastDay>& forecast);
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
    void windRoseDataChanged();

    // Sparkline history signals
    void temperatureHistoryChanged();
    void feelsLikeHistoryChanged();
    void humidityHistoryChanged();
    void dewPointHistoryChanged();
    void windSpeedHistoryChanged();
    void rainRateHistoryChanged();
    void pressureHistoryChanged();
    void uvIndexHistoryChanged();
    void solarRadHistoryChanged();

    // PurpleAir signals
    void aqiChanged(double value);
    void pm25Changed(double value);
    void pm10Changed(double value);
    void purpleAirStaleChanged(bool stale);
    void aqiHistoryChanged();

    // Forecast signal
    void forecastDataChanged();

private:
    void clearAllValues();
    void clearPurpleAirValues();
    void markUpdated();
    void recordWindSample(int dir, double speed);
    void recordSparklineSample(double* ring, int& head, int& count, double value);
    QVariantList sparklineToList(const double* ring, int head, int count) const;

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

    // Sparkline ring buffers — one per outdoor sensor
    double m_tempSparkline[kSparklineCapacity] = {};
    int m_tempSparklineHead = 0;
    int m_tempSparklineCount = 0;

    double m_feelsLikeSparkline[kSparklineCapacity] = {};
    int m_feelsLikeSparklineHead = 0;
    int m_feelsLikeSparklineCount = 0;

    double m_humSparkline[kSparklineCapacity] = {};
    int m_humSparklineHead = 0;
    int m_humSparklineCount = 0;

    double m_dewPointSparkline[kSparklineCapacity] = {};
    int m_dewPointSparklineHead = 0;
    int m_dewPointSparklineCount = 0;

    double m_windSparkline[kSparklineCapacity] = {};
    int m_windSparklineHead = 0;
    int m_windSparklineCount = 0;

    double m_rainRateSparkline[kSparklineCapacity] = {};
    int m_rainRateSparklineHead = 0;
    int m_rainRateSparklineCount = 0;

    double m_pressureSparkline[kSparklineCapacity] = {};
    int m_pressureSparklineHead = 0;
    int m_pressureSparklineCount = 0;

    double m_uvSparkline[kSparklineCapacity] = {};
    int m_uvSparklineHead = 0;
    int m_uvSparklineCount = 0;

    double m_solarRadSparkline[kSparklineCapacity] = {};
    int m_solarRadSparklineHead = 0;
    int m_solarRadSparklineCount = 0;

    // Wind rose histogram (16 compass bins, each 22.5°) with rolling window
    static constexpr int kWindBins = 16;
    static constexpr int kMaxWindSamples = 720; // ~30 min at 2.5s UDP rate
    int m_windBinCount[kWindBins] = {};
    double m_windBinTotalSpeed[kWindBins] = {};

    // Ring buffer for rolling window eviction
    struct WindSample { int bin; double speed; };
    WindSample m_windRing[kMaxWindSamples] = {};
    int m_windRingHead = 0;  // next write position
    int m_windRingCount = 0; // number of samples stored

    // PurpleAir fields
    double m_aqi = 0.0;
    double m_pm25 = 0.0;
    double m_pm10 = 0.0;

    // PurpleAir staleness — independent from weather station staleness
    bool m_purpleAirStale = false;
    bool m_hasPurpleAirUpdate = false;
    qint64 m_lastPurpleAirElapsed = 0;

    // Forecast data (retained until next successful fetch — no staleness clearing)
    QVector<ForecastDay> m_forecast;

    // AQI sparkline ring buffer (24h at 30s PurpleAir cadence)
    static constexpr int kAqiSparklineCapacity = 2880;
    double m_aqiSparkline[kAqiSparklineCapacity] = {};
    int m_aqiSparklineHead = 0;
    int m_aqiSparklineCount = 0;

    // Staleness tracking
    static constexpr int kStalenessMs = 30000;
    bool m_hasReceivedUpdate = false;
    qint64 m_lastUpdateElapsed = 0; // Value from elapsed provider at time of last update
    QTimer* m_stalenessTimer = nullptr;
    std::function<qint64()> m_elapsedProvider;
    QElapsedTimer m_wallClock; // Used when no provider is injected
};
