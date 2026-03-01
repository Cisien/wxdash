#include "network/JsonParser.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

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
    r.pressureTrend = obj["bar_trend"].toInt();
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

} // namespace JsonParser
