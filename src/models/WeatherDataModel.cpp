#include "models/WeatherDataModel.h"

#include <QDir>
#include <QFile>
#include <QTimer>

#include <cstring>

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
    // WeatherLink API reports dir=0, speed=0 when calm — use bin=-1 sentinel
    int bin;
    if (dir == 0 && qFuzzyCompare(speed + 1.0, 1.0))
        bin = -1; // calm/idle — occupies ring slot but no directional bin
    else
        bin = qBound(0, qRound(dir / 22.5) % kWindBins, kWindBins - 1);

    // Evict oldest sample if ring buffer is full
    if (m_windRingCount >= kMaxWindSamples) {
        const auto& old = m_windRing[m_windRingHead];
        if (old.bin >= 0) {
            m_windBinCount[old.bin]--;
            m_windBinTotalSpeed[old.bin] -= old.speed;
        }
    }

    // Write new sample into ring buffer
    m_windRing[m_windRingHead] = {bin, speed};
    m_windRingHead = (m_windRingHead + 1) % kMaxWindSamples;
    if (m_windRingCount < kMaxWindSamples)
        m_windRingCount++;

    // Update bin totals (skip calm samples)
    if (bin >= 0) {
        m_windBinCount[bin]++;
        m_windBinTotalSpeed[bin] += speed;
    }
    emit windRoseDataChanged();
}

QVariantList WeatherDataModel::windRoseData() const {
    // Compute recent average speed per bin by walking the ring buffer backwards
    // collecting the last kRecentSpeedSamples speeds for each directional bin
    double recentSum[kWindBins] = {};
    int recentCount[kWindBins] = {};

    for (int i = 0; i < m_windRingCount; i++) {
        // Walk from newest to oldest
        int idx = (m_windRingHead - 1 - i + kMaxWindSamples) % kMaxWindSamples;
        const auto& sample = m_windRing[idx];
        if (sample.bin >= 0 && recentCount[sample.bin] < kRecentSpeedSamples) {
            recentSum[sample.bin] += sample.speed;
            recentCount[sample.bin]++;
        }
    }

    QVariantList list;
    list.reserve(kWindBins);
    for (int i = 0; i < kWindBins; i++) {
        QVariantMap bin;
        bin[QStringLiteral("count")] = m_windBinCount[i];
        bin[QStringLiteral("avgSpeed")] =
            m_windBinCount[i] > 0 ? m_windBinTotalSpeed[i] / m_windBinCount[i] : 0.0;
        bin[QStringLiteral("recentAvgSpeed")] =
            recentCount[i] > 0 ? recentSum[i] / recentCount[i] : 0.0;
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

double WeatherDataModel::windRoseDirectionalFraction() const {
    if (m_windRingCount == 0) return 0.0;
    int totalDirectional = 0;
    for (int i = 0; i < kWindBins; i++)
        totalDirectional += m_windBinCount[i];
    return double(totalDirectional) / m_windRingCount;
}

void WeatherDataModel::recordSample(SparklineId id, double value) {
    auto& sl = m_sparklines[id];
    sl.data[sl.head] = value;
    sl.head = (sl.head + 1) % kSparklineCapacity;
    if (sl.count < kSparklineCapacity) sl.count++;
    rebuildCache(id);
}

void WeatherDataModel::rebuildCache(SparklineId id) {
    auto& sl = m_sparklines[id];
    sl.cache.clear();
    if (sl.count == 0) return;
    const int stride = qMax(1, sl.count / kMaxSparklinePoints);
    sl.cache.reserve(sl.count / stride + 1);
    for (int i = 0; i < sl.count; i += stride) {
        const int idx = (sl.head - sl.count + i + kSparklineCapacity) % kSparklineCapacity;
        sl.cache.append(sl.data[idx]);
    }
}

void WeatherDataModel::rebuildPressureMbarCache() {
    const auto& pc = m_sparklines[SL_Pressure].cache;
    m_pressureMbarCache.clear();
    m_pressureMbarCache.reserve(pc.size());
    for (const auto& v : pc)
        m_pressureMbarCache.append(v.toDouble() * 33.8639);
}

void WeatherDataModel::rebuildAllCaches() {
    for (int i = 0; i < SL_Count; i++)
        rebuildCache(static_cast<SparklineId>(i));
    rebuildPressureMbarCache();
    // AQI has a different capacity — rebuild inline
    m_aqiHistoryCache.clear();
    if (m_aqiSparklineCount > 0) {
        const int stride = qMax(1, m_aqiSparklineCount / kMaxSparklinePoints);
        m_aqiHistoryCache.reserve(m_aqiSparklineCount / stride + 1);
        for (int i = 0; i < m_aqiSparklineCount; i += stride) {
            const int idx = (m_aqiSparklineHead - m_aqiSparklineCount + i + kAqiSparklineCapacity) % kAqiSparklineCapacity;
            m_aqiHistoryCache.append(m_aqiSparkline[idx]);
        }
    }
}

// ---------------------------------------------------------------------------
// Sparkline persistence
// ---------------------------------------------------------------------------

static constexpr quint32 kSparklineMagic = 0x57584448; // "WXDH"
static constexpr quint32 kSparklineVersion = 2;

void WeatherDataModel::saveSparklineData(const QString& path) const {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return;

    QDataStream out(&file);
    out.setVersion(QDataStream::Qt_6_0);
    out << kSparklineMagic << kSparklineVersion;

    // Helper: save ring buffer as chronological sequence
    auto saveRing = [&](const double* ring, int head, int count, int capacity) {
        out << qint32(count);
        for (int i = 0; i < count; i++) {
            int idx = (head - count + i + capacity) % capacity;
            out << ring[idx];
        }
    };

    // 9 weather sparklines
    for (int i = 0; i < SL_Count; i++)
        saveRing(m_sparklines[i].data, m_sparklines[i].head, m_sparklines[i].count, kSparklineCapacity);

    // AQI sparkline (different capacity)
    saveRing(m_aqiSparkline, m_aqiSparklineHead, m_aqiSparklineCount, kAqiSparklineCapacity);

    // Wind rose ring buffer (version 2+)
    out << qint32(m_windRingCount);
    for (int i = 0; i < m_windRingCount; i++) {
        int idx = (m_windRingHead - m_windRingCount + i + kMaxWindSamples) % kMaxWindSamples;
        out << qint32(m_windRing[idx].bin) << m_windRing[idx].speed;
    }
}

void WeatherDataModel::loadSparklineData(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return;

    QDataStream in(&file);
    in.setVersion(QDataStream::Qt_6_0);

    quint32 magic, version;
    in >> magic >> version;
    if (magic != kSparklineMagic || version < 1 || version > kSparklineVersion)
        return;

    // Helper: load chronological sequence into ring buffer
    auto loadRing = [&](double* ring, int& head, int& count, int capacity) {
        qint32 savedCount;
        in >> savedCount;
        if (in.status() != QDataStream::Ok || savedCount < 0)
            return;
        savedCount = qMin(savedCount, qint32(capacity));
        for (int i = 0; i < savedCount; i++) {
            double val;
            in >> val;
            if (in.status() != QDataStream::Ok)
                return;
            ring[i] = val;
        }
        head = savedCount % capacity;
        count = savedCount;
    };

    // 9 weather sparklines
    for (int i = 0; i < SL_Count; i++)
        loadRing(m_sparklines[i].data, m_sparklines[i].head, m_sparklines[i].count, kSparklineCapacity);

    // AQI sparkline
    loadRing(m_aqiSparkline, m_aqiSparklineHead, m_aqiSparklineCount, kAqiSparklineCapacity);

    // Wind rose ring buffer (version 2+)
    if (version >= 2) {
        qint32 windCount;
        in >> windCount;
        if (in.status() == QDataStream::Ok && windCount > 0) {
            windCount = qMin(windCount, qint32(kMaxWindSamples));
            memset(m_windBinCount, 0, sizeof(m_windBinCount));
            memset(m_windBinTotalSpeed, 0, sizeof(m_windBinTotalSpeed));
            for (int i = 0; i < windCount; i++) {
                qint32 bin;
                double speed;
                in >> bin >> speed;
                if (in.status() != QDataStream::Ok)
                    break;
                if (bin >= 0)
                    bin = qBound(0, int(bin), kWindBins - 1);
                // bin < 0 means calm sample (sentinel -1) — keep as-is
                m_windRing[i] = {int(bin), speed};
                if (bin >= 0) {
                    m_windBinCount[bin]++;
                    m_windBinTotalSpeed[bin] += speed;
                }
            }
            m_windRingHead = windCount % kMaxWindSamples;
            m_windRingCount = windCount;
        }
    }

    // Rebuild all pre-decimated caches before notifying QML
    rebuildAllCaches();

    // Notify QML that all histories are available
    emit windRoseDataChanged();
    emit temperatureHistoryChanged();
    emit feelsLikeHistoryChanged();
    emit humidityHistoryChanged();
    emit dewPointHistoryChanged();
    emit windSpeedHistoryChanged();
    emit rainRateHistoryChanged();
    emit pressureHistoryChanged();
    emit uvIndexHistoryChanged();
    emit solarRadHistoryChanged();
    emit aqiHistoryChanged();
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
    if (!qFuzzyIsNull(m_pressureTrend)) {
        m_pressureTrend = 0.0;
        emit pressureTrendChanged(0.0);
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
    // Skip bogus dir=0 when speed=0 (WeatherLink calm convention)
    if (m_windDir != r.windDirLast
        && !(r.windDirLast == 0 && qFuzzyCompare(r.windSpeedLast + 1.0, 1.0))) {
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

    // Record sparkline samples and notify QML
    recordSample(SL_Temperature, m_temperature);
    emit temperatureHistoryChanged();

    double feelsLike = m_temperature;
    if (m_temperature >= 80.0 && m_humidity >= 40.0) feelsLike = m_heatIndex;
    else if (m_temperature <= 50.0 && m_windSpeed >= 3.0) feelsLike = m_windChill;
    recordSample(SL_FeelsLike, feelsLike);
    emit feelsLikeHistoryChanged();

    recordSample(SL_Humidity, m_humidity);       emit humidityHistoryChanged();
    recordSample(SL_DewPoint, m_dewPoint);       emit dewPointHistoryChanged();
    recordSample(SL_WindSpeed, m_windSpeed);     emit windSpeedHistoryChanged();
    recordSample(SL_RainRate, m_rainRate);       emit rainRateHistoryChanged();
    recordSample(SL_UvIndex, m_uvIndex);         emit uvIndexHistoryChanged();
    recordSample(SL_SolarRad, m_solarRad);       emit solarRadHistoryChanged();
}

void WeatherDataModel::applyBarUpdate(const BarReading& r) {
    markUpdated();

    if (!qFuzzyCompare(m_pressure, r.pressureSeaLevel)) {
        m_pressure = r.pressureSeaLevel;
        emit pressureChanged(m_pressure);
    }
    if (!qFuzzyCompare(m_pressureTrend, r.pressureTrend)) {
        m_pressureTrend = r.pressureTrend;
        emit pressureTrendChanged(m_pressureTrend);
    }

    recordSample(SL_Pressure, m_pressure);
    rebuildPressureMbarCache();
    emit pressureHistoryChanged();
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
    // Skip bogus dir=0 when speed=0 (WeatherLink calm convention)
    if (m_windDir != r.windDirLast
        && !(r.windDirLast == 0 && qFuzzyCompare(r.windSpeedLast + 1.0, 1.0))) {
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

// ---------------------------------------------------------------------------
// PurpleAir methods
// ---------------------------------------------------------------------------

void WeatherDataModel::applyPurpleAirUpdate(const PurpleAirReading& r) {
    // Silent recovery from staleness
    if (m_purpleAirStale) {
        m_purpleAirStale = false;
        emit purpleAirStaleChanged(false);
    }

    // Mark PurpleAir as updated (for staleness tracking)
    if (m_elapsedProvider) {
        m_lastPurpleAirElapsed = m_elapsedProvider();
    } else {
        if (!m_wallClock.isValid()) m_wallClock.start();
        m_lastPurpleAirElapsed = m_wallClock.elapsed();
    }
    if (!m_hasPurpleAirUpdate) {
        m_hasPurpleAirUpdate = true;
    }

    if (!qFuzzyCompare(m_aqi + 1.0, r.aqi + 1.0)) {
        m_aqi = r.aqi;
        emit aqiChanged(m_aqi);
    }
    if (!qFuzzyCompare(m_pm25 + 1.0, r.pm25avg + 1.0)) {
        m_pm25 = r.pm25avg;
        emit pm25Changed(m_pm25);
    }
    if (!qFuzzyCompare(m_pm10 + 1.0, r.pm10 + 1.0)) {
        m_pm10 = r.pm10;
        emit pm10Changed(m_pm10);
    }

    // Record AQI sparkline sample (different capacity than weather sparklines)
    m_aqiSparkline[m_aqiSparklineHead] = m_aqi;
    m_aqiSparklineHead = (m_aqiSparklineHead + 1) % kAqiSparklineCapacity;
    if (m_aqiSparklineCount < kAqiSparklineCapacity) m_aqiSparklineCount++;
    m_aqiHistoryCache.clear();
    const int aqiStride = qMax(1, m_aqiSparklineCount / kMaxSparklinePoints);
    m_aqiHistoryCache.reserve(m_aqiSparklineCount / aqiStride + 1);
    for (int i = 0; i < m_aqiSparklineCount; i += aqiStride) {
        const int idx = (m_aqiSparklineHead - m_aqiSparklineCount + i + kAqiSparklineCapacity) % kAqiSparklineCapacity;
        m_aqiHistoryCache.append(m_aqiSparkline[idx]);
    }
    emit aqiHistoryChanged();
}


// ---------------------------------------------------------------------------
// Forecast methods
// ---------------------------------------------------------------------------

QVariantList WeatherDataModel::forecastData() const {
    QVariantList list;
    for (const auto& day : m_forecast) {
        QVariantMap map;
        map[QStringLiteral("high")]     = day.high;
        map[QStringLiteral("low")]      = day.low;
        map[QStringLiteral("precip")]   = day.precip;
        map[QStringLiteral("iconCode")] = day.iconCode;
        list.append(map);
    }
    return list;
}

void WeatherDataModel::applyForecastUpdate(const QVector<ForecastDay>& forecast) {
    m_forecast = forecast;
    emit forecastDataChanged();
}

void WeatherDataModel::clearPurpleAirValues() {
    if (!qFuzzyCompare(m_aqi + 1.0, 1.0)) {
        m_aqi = 0.0;
        emit aqiChanged(0.0);
    }
    if (!qFuzzyCompare(m_pm25 + 1.0, 1.0)) {
        m_pm25 = 0.0;
        emit pm25Changed(0.0);
    }
    if (!qFuzzyCompare(m_pm10 + 1.0, 1.0)) {
        m_pm10 = 0.0;
        emit pm10Changed(0.0);
    }
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

    // PurpleAir staleness check — independent of weather station
    if (m_hasPurpleAirUpdate) {
        const qint64 paSinceUpdate = currentElapsed - m_lastPurpleAirElapsed;
        if (paSinceUpdate > kStalenessMs && !m_purpleAirStale) {
            m_purpleAirStale = true;
            clearPurpleAirValues();
            emit purpleAirStaleChanged(true);
        }
    }
}
