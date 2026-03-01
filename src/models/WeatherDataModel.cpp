#include "models/WeatherDataModel.h"

#include <QTimer>

WeatherDataModel::WeatherDataModel(QObject* parent, std::function<qint64()> elapsedProvider)
    : QObject(parent), m_elapsedProvider(std::move(elapsedProvider)) {
    m_stalenessTimer = new QTimer(this);
    m_stalenessTimer->setInterval(5000); // check every 5s
    connect(m_stalenessTimer, &QTimer::timeout, this, &WeatherDataModel::checkStaleness);
    // Timer starts only after the first update is received (see markUpdated)
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

void WeatherDataModel::markUpdated() {
    if (m_elapsedProvider) {
        // Injectable clock: record current provider value as "time of last update"
        m_lastUpdateElapsed = m_elapsedProvider();
    } else {
        // Real wall clock: start on first update, then record elapsed
        if (!m_wallClock.isValid()) {
            m_wallClock.start();
        }
        m_lastUpdateElapsed = m_wallClock.elapsed();
    }

    if (!m_hasReceivedUpdate) {
        m_hasReceivedUpdate = true;
        m_stalenessTimer->start();
    }
}

void WeatherDataModel::recordWindSample(int dir, double speed) {
    int bin = qBound(0, qRound(dir / 22.5) % kWindBins, kWindBins - 1);
    m_windBinCount[bin]++;
    m_windBinTotalSpeed[bin] += speed;
    emit windRoseDataChanged();
}

QVariantList WeatherDataModel::windRoseData() const {
    QVariantList list;
    list.reserve(kWindBins);
    for (int i = 0; i < kWindBins; i++) {
        QVariantMap bin;
        bin[QStringLiteral("count")] = m_windBinCount[i];
        bin[QStringLiteral("avgSpeed")] =
            m_windBinCount[i] > 0 ? m_windBinTotalSpeed[i] / m_windBinCount[i] : 0.0;
        list.append(bin);
    }
    return list;
}

int WeatherDataModel::windRoseMaxCount() const {
    int max = 0;
    for (int i = 0; i < kWindBins; i++)
        max = qMax(max, m_windBinCount[i]);
    return max;
}

void WeatherDataModel::clearAllValues() {
    // Clear each field to zero, emitting changed signals only for non-zero fields.
    // Uses qFuzzyCompare with offset to handle the zero-comparison edge case correctly.

    auto clearDouble = [this](double& field, auto signal) {
        if (!qFuzzyCompare(field + 1.0, 1.0)) {
            field = 0.0;
            emit(this->*signal)(0.0);
        }
    };

    clearDouble(m_temperature, &WeatherDataModel::temperatureChanged);
    clearDouble(m_humidity, &WeatherDataModel::humidityChanged);
    clearDouble(m_dewPoint, &WeatherDataModel::dewPointChanged);
    clearDouble(m_heatIndex, &WeatherDataModel::heatIndexChanged);
    clearDouble(m_windChill, &WeatherDataModel::windChillChanged);
    clearDouble(m_windSpeed, &WeatherDataModel::windSpeedChanged);
    clearDouble(m_windGust, &WeatherDataModel::windGustChanged);
    clearDouble(m_rainRate, &WeatherDataModel::rainRateChanged);
    clearDouble(m_rainfallDaily, &WeatherDataModel::rainfallDailyChanged);
    clearDouble(m_pressure, &WeatherDataModel::pressureChanged);
    clearDouble(m_uvIndex, &WeatherDataModel::uvIndexChanged);
    clearDouble(m_solarRad, &WeatherDataModel::solarRadChanged);
    clearDouble(m_tempIn, &WeatherDataModel::tempInChanged);
    clearDouble(m_humIn, &WeatherDataModel::humInChanged);
    clearDouble(m_dewPointIn, &WeatherDataModel::dewPointInChanged);

    if (m_windDir != 0) {
        m_windDir = 0;
        emit windDirChanged(0);
    }
    if (m_pressureTrend != 0) {
        m_pressureTrend = 0;
        emit pressureTrendChanged(0);
    }
}

// ---------------------------------------------------------------------------
// Public slots
// ---------------------------------------------------------------------------

void WeatherDataModel::applyIssUpdate(const IssReading& r) {
    // Silent recovery: clear stale flag before processing new data
    if (m_sourceStale) {
        m_sourceStale = false;
        emit sourceStaleChanged(false);
    }

    markUpdated();

    if (!qFuzzyCompare(m_temperature, r.temperature)) {
        m_temperature = r.temperature;
        emit temperatureChanged(m_temperature);
    }
    if (!qFuzzyCompare(m_humidity, r.humidity)) {
        m_humidity = r.humidity;
        emit humidityChanged(m_humidity);
    }
    if (!qFuzzyCompare(m_dewPoint, r.dewPoint)) {
        m_dewPoint = r.dewPoint;
        emit dewPointChanged(m_dewPoint);
    }
    if (!qFuzzyCompare(m_heatIndex, r.heatIndex)) {
        m_heatIndex = r.heatIndex;
        emit heatIndexChanged(m_heatIndex);
    }
    if (!qFuzzyCompare(m_windChill, r.windChill)) {
        m_windChill = r.windChill;
        emit windChillChanged(m_windChill);
    }
    if (!qFuzzyCompare(m_windSpeed, r.windSpeedLast)) {
        m_windSpeed = r.windSpeedLast;
        emit windSpeedChanged(m_windSpeed);
    }
    if (m_windDir != r.windDirLast) {
        m_windDir = r.windDirLast;
        emit windDirChanged(m_windDir);
    }
    if (!qFuzzyCompare(m_windGust, r.windSpeedHi10)) {
        m_windGust = r.windSpeedHi10;
        emit windGustChanged(m_windGust);
    }
    if (!qFuzzyCompare(m_solarRad, r.solarRad)) {
        m_solarRad = r.solarRad;
        emit solarRadChanged(m_solarRad);
    }
    if (!qFuzzyCompare(m_uvIndex, r.uvIndex)) {
        m_uvIndex = r.uvIndex;
        emit uvIndexChanged(m_uvIndex);
    }
    if (!qFuzzyCompare(m_rainRate, r.rainRateLast)) {
        m_rainRate = r.rainRateLast;
        emit rainRateChanged(m_rainRate);
    }
    if (!qFuzzyCompare(m_rainfallDaily, r.rainfallDaily)) {
        m_rainfallDaily = r.rainfallDaily;
        emit rainfallDailyChanged(m_rainfallDaily);
    }

    recordWindSample(r.windDirLast, r.windSpeedLast);
}

void WeatherDataModel::applyBarUpdate(const BarReading& r) {
    markUpdated();

    if (!qFuzzyCompare(m_pressure, r.pressureSeaLevel)) {
        m_pressure = r.pressureSeaLevel;
        emit pressureChanged(m_pressure);
    }
    if (m_pressureTrend != r.pressureTrend) {
        m_pressureTrend = r.pressureTrend;
        emit pressureTrendChanged(m_pressureTrend);
    }
}

void WeatherDataModel::applyIndoorUpdate(const IndoorReading& r) {
    markUpdated();

    if (!qFuzzyCompare(m_tempIn, r.tempIn)) {
        m_tempIn = r.tempIn;
        emit tempInChanged(m_tempIn);
    }
    if (!qFuzzyCompare(m_humIn, r.humIn)) {
        m_humIn = r.humIn;
        emit humInChanged(m_humIn);
    }
    if (!qFuzzyCompare(m_dewPointIn, r.dewPointIn)) {
        m_dewPointIn = r.dewPointIn;
        emit dewPointInChanged(m_dewPointIn);
    }
}

void WeatherDataModel::applyUdpUpdate(const UdpReading& r) {
    // Silent recovery: clear stale flag before processing new data
    if (m_sourceStale) {
        m_sourceStale = false;
        emit sourceStaleChanged(false);
    }

    markUpdated();

    // UDP updates overwrite the fast-changing wind/rain fields
    if (!qFuzzyCompare(m_windSpeed, r.windSpeedLast)) {
        m_windSpeed = r.windSpeedLast;
        emit windSpeedChanged(m_windSpeed);
    }
    if (m_windDir != r.windDirLast) {
        m_windDir = r.windDirLast;
        emit windDirChanged(m_windDir);
    }
    if (!qFuzzyCompare(m_windGust, r.windSpeedHi10)) {
        m_windGust = r.windSpeedHi10;
        emit windGustChanged(m_windGust);
    }
    if (!qFuzzyCompare(m_rainRate, r.rainRateLast)) {
        m_rainRate = r.rainRateLast;
        emit rainRateChanged(m_rainRate);
    }
    if (!qFuzzyCompare(m_rainfallDaily, r.rainfallDaily)) {
        m_rainfallDaily = r.rainfallDaily;
        emit rainfallDailyChanged(m_rainfallDaily);
    }

    recordWindSample(r.windDirLast, r.windSpeedLast);
}

void WeatherDataModel::checkStaleness() {
    // No false positive: never stale before the first update
    if (!m_hasReceivedUpdate) {
        return;
    }

    // Get current elapsed time from the appropriate clock
    const qint64 currentElapsed = m_elapsedProvider ? m_elapsedProvider() : m_wallClock.elapsed();
    const qint64 elapsedSinceUpdate = currentElapsed - m_lastUpdateElapsed;

    if (elapsedSinceUpdate > kStalenessMs && !m_sourceStale) {
        m_sourceStale = true;
        clearAllValues();
        emit sourceStaleChanged(true);
        // Note: recovery happens in applyIssUpdate/applyUdpUpdate when new data arrives
    }
}
