#include "network/JsonParser.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>

namespace JsonParser {

double rainSizeToInches(int rainSize)
{
    switch (rainSize) {
    case 1:
        return 0.01;
    case 2:
        return 0.2 / 25.4; // 0.2mm → inches
    case 3:
        return 0.1 / 25.4; // 0.1mm → inches
    case 4:
        return 0.001;
    default:
        return 0.01; // fallback to most common size
    }
}

static IssReading parseIssObject(const QJsonObject &obj)
{
    IssReading r;
    r.temperature = obj["temp"].toDouble();
    r.humidity = obj["hum"].toDouble();
    r.dewPoint = obj["dew_point"].toDouble();
    r.heatIndex = obj["heat_index"].toDouble();
    r.windChill = obj["wind_chill"].toDouble();
    r.windSpeedLast = obj["wind_speed_last"].toDouble();
    r.windDirLast = obj["wind_dir_last"].toInt();
    r.windSpeedHi10 = obj["wind_speed_hi_last_10_min"].toDouble();
    r.solarRad = obj["solar_rad"].toDouble();
    r.uvIndex = obj["uv_index"].toDouble();
    r.rainSize = obj["rain_size"].toInt(1);

    const double factor = rainSizeToInches(r.rainSize);
    r.rainRateLast = obj["rain_rate_last"].toDouble() * factor;
    r.rainfallDaily = obj["rainfall_daily"].toDouble() * factor;

    return r;
}

static BarReading parseBarObject(const QJsonObject &obj)
{
    BarReading r;
    r.pressureSeaLevel = obj["bar_sea_level"].toDouble();
    r.pressureTrend = obj["bar_trend"].toDouble();
    return r;
}

static IndoorReading parseIndoorObject(const QJsonObject &obj)
{
    IndoorReading r;
    r.tempIn = obj["temp_in"].toDouble();
    r.humIn = obj["hum_in"].toDouble();
    r.dewPointIn = obj["dew_point_in"].toDouble();
    return r;
}

ParsedConditions parseCurrentConditions(const QByteArray &data)
{
    ParsedConditions result;

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        return result; // malformed JSON — return all-nullopt
    }

    const QJsonObject root = doc.object();
    const QJsonObject dataObj = root["data"].toObject();
    const QJsonArray conditions = dataObj["conditions"].toArray();

    if (conditions.isEmpty()) {
        return result;
    }

    // Iterate and route by data_structure_type — NEVER by array index
    for (const QJsonValue &val : conditions) {
        if (!val.isObject()) {
            continue;
        }
        const QJsonObject obj = val.toObject();
        const int type = obj["data_structure_type"].toInt(-1);

        switch (type) {
        case 1:
            result.iss = parseIssObject(obj);
            break;
        case 3:
            result.bar = parseBarObject(obj);
            break;
        case 4:
            result.indoor = parseIndoorObject(obj);
            break;
        default:
            // Unknown or unsupported type — silently skip per locked decision
            break;
        }
    }

    return result;
}

std::optional<UdpReading> parseUdpDatagram(const QByteArray &data)
{
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        return std::nullopt;
    }

    const QJsonObject root = doc.object();
    // UDP uses top-level "conditions" (not "data.conditions")
    const QJsonArray conditions = root["conditions"].toArray();

    for (const QJsonValue &val : conditions) {
        if (!val.isObject()) {
            continue;
        }
        const QJsonObject obj = val.toObject();
        const int type = obj["data_structure_type"].toInt(-1);

        if (type != 1) {
            continue;
        }

        UdpReading r;
        r.windSpeedLast = obj["wind_speed_last"].toDouble();
        r.windDirLast = obj["wind_dir_last"].toInt();
        r.windSpeedHi10 = obj["wind_speed_hi_last_10_min"].toDouble();
        r.rainSize = obj["rain_size"].toInt(1);

        const double factor = rainSizeToInches(r.rainSize);
        r.rainRateLast = obj["rain_rate_last"].toDouble() * factor;
        r.rainfallDaily = obj["rainfall_daily"].toDouble() * factor;

        return r; // return the first type-1 entry found
    }

    return std::nullopt; // no type-1 entry found
}


int calculateAqi(double pm25) {
    if (pm25 < 0.0) return 0;

    struct Breakpoint { double concLo, concHi; int aqiLo, aqiHi; };
    static constexpr Breakpoint kPm25Breakpoints[] = {
        {  0.0,   9.0,   0,  50 },   // Good (2024: was 12.0)
        {  9.1,  35.4,  51, 100 },   // Moderate
        { 35.5,  55.4, 101, 150 },   // Unhealthy for Sensitive Groups
        { 55.5, 150.4, 151, 200 },   // Unhealthy (2024: was 125.4)
        {150.5, 250.4, 201, 300 },   // Very Unhealthy (2024: was 225.4)
        {250.5, 500.4, 301, 500 },   // Hazardous (2024: was 325.4)
    };

    for (const auto& bp : kPm25Breakpoints) {
        if (pm25 >= bp.concLo && pm25 <= bp.concHi) {
            return qRound(
                ((pm25 - bp.concLo) / (bp.concHi - bp.concLo))
                * (bp.aqiHi - bp.aqiLo) + bp.aqiLo
            );
        }
    }
    // Above 500.4 — clamp to 500
    return (pm25 > 500.4) ? 500 : 0;
}

PurpleAirReading parsePurpleAirJson(const QByteArray &data) {
    PurpleAirReading result;
    QJsonParseError err;
    auto doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) return result;

    auto root = doc.object();
    result.pm25_a  = root.value("pm2_5_atm").toDouble(0.0);
    result.pm25_b  = root.value("pm2_5_atm_b").toDouble(0.0);
    result.pm10    = root.value("pm10_0_atm").toDouble(0.0);
    result.pm25avg = (result.pm25_a + result.pm25_b) / 2.0;
    result.aqi     = calculateAqi(result.pm25avg);
    return result;
}

// ---------------------------------------------------------------------------
// NWS forecast parsing
// ---------------------------------------------------------------------------

static QString extractIconCode(const QString &iconUrl) {
    // "https://api.weather.gov/icons/land/day/tsra_hi,40?size=medium" -> "tsra_hi"
    QUrl url(iconUrl);
    QString last = url.path().section('/', -1);  // "tsra_hi,40"
    return last.section(',', 0, 0);               // "tsra_hi"
}

QVector<ForecastDay> parseForecast(const QByteArray &data) {
    QVector<ForecastDay> result;

    QJsonParseError err;
    auto doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) return result;

    auto periods = doc.object()["properties"].toObject()["periods"].toArray();
    if (periods.isEmpty()) return result;

    ForecastDay current;
    bool hasHigh = false;

    for (const auto &val : periods) {
        if (result.size() >= 3) break;
        auto obj = val.toObject();
        bool isDaytime = obj["isDaytime"].toBool();
        int temp = obj["temperature"].toInt();
        // probabilityOfPrecipitation.value may be null — toInt(0) handles null as 0
        int precip = obj["probabilityOfPrecipitation"].toObject()["value"].toInt(0);
        QString iconCode = extractIconCode(obj["icon"].toString());

        if (isDaytime) {
            // Start of a new day — if we had an incomplete day, push it
            if (hasHigh) {
                result.append(current);
                current = ForecastDay{};
                hasHigh = false;
                if (result.size() >= 3) break;
            }
            current.high = temp;
            current.precip = precip;
            current.iconCode = iconCode;
            hasHigh = true;
        } else {
            // Nighttime period
            if (!hasHigh) {
                // Tonight only — no daytime period seen yet (afternoon fetch)
                current.high = -999;
                current.iconCode = iconCode;  // Use nighttime icon
            }
            current.low = temp;
            // Use max of day and night precip
            if (precip > current.precip) current.precip = precip;
            result.append(current);
            current = ForecastDay{};
            hasHigh = false;
        }
    }
    // If last period was daytime with no following night, append anyway
    if (hasHigh && result.size() < 3) result.append(current);

    return result;
}

} // namespace JsonParser
