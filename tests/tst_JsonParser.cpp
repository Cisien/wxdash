#include "network/JsonParser.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QtTest/QtTest>
#include <cmath>

class TstJsonParser : public QObject {
    Q_OBJECT

private slots:
    // -----------------------------------------------------------------------
    // rainSizeToInches tests
    // -----------------------------------------------------------------------
    void rainSize_1_returns_001()
    {
        QCOMPARE(JsonParser::rainSizeToInches(1), 0.01);
    }

    void rainSize_2_returns_0point2mm_in_inches()
    {
        const double expected = 0.2 / 25.4;
        QVERIFY(std::fabs(JsonParser::rainSizeToInches(2) - expected) < 1e-10);
    }

    void rainSize_3_returns_0point1mm_in_inches()
    {
        const double expected = 0.1 / 25.4;
        QVERIFY(std::fabs(JsonParser::rainSizeToInches(3) - expected) < 1e-10);
    }

    void rainSize_4_returns_0point001()
    {
        QCOMPARE(JsonParser::rainSizeToInches(4), 0.001);
    }

    void rainSize_unknown_returns_fallback()
    {
        // Unknown value falls back to 0.01 (rain_size 1)
        QCOMPARE(JsonParser::rainSizeToInches(99), 0.01);
        QCOMPARE(JsonParser::rainSizeToInches(0), 0.01);
        QCOMPARE(JsonParser::rainSizeToInches(-1), 0.01);
    }

    // -----------------------------------------------------------------------
    // parseCurrentConditions: ordering tests
    // -----------------------------------------------------------------------
    void parseCurrentConditions_type1First_type3Second()
    {
        // Proves parser works with ISS before barometer
        const QByteArray json = R"({
            "data": {
                "conditions": [
                    {
                        "data_structure_type": 1,
                        "temp": 72.5,
                        "hum": 55.0,
                        "dew_point": 55.3,
                        "heat_index": 73.1,
                        "wind_chill": 72.5,
                        "wind_speed_last": 8.0,
                        "wind_dir_last": 180,
                        "wind_speed_hi_last_10_min": 12.0,
                        "solar_rad": 450.0,
                        "uv_index": 3.2,
                        "rain_size": 1,
                        "rain_rate_last": 0,
                        "rainfall_daily": 0
                    },
                    {
                        "data_structure_type": 3,
                        "bar_sea_level": 29.92,
                        "bar_trend": 0
                    }
                ]
            }
        })";

        auto result = JsonParser::parseCurrentConditions(json);
        QVERIFY(result.iss.has_value());
        QVERIFY(result.bar.has_value());
        QVERIFY(!result.indoor.has_value());

        QCOMPARE(result.iss->temperature, 72.5);
        QCOMPARE(result.bar->pressureSeaLevel, 29.92);
    }

    void parseCurrentConditions_type3First_type1Second()
    {
        // Proves parser does NOT assume array index — barometer before ISS
        const QByteArray json = R"({
            "data": {
                "conditions": [
                    {
                        "data_structure_type": 3,
                        "bar_sea_level": 30.01,
                        "bar_trend": 1
                    },
                    {
                        "data_structure_type": 1,
                        "temp": 65.0,
                        "hum": 60.0,
                        "dew_point": 50.0,
                        "heat_index": 65.0,
                        "wind_chill": 65.0,
                        "wind_speed_last": 5.0,
                        "wind_dir_last": 270,
                        "wind_speed_hi_last_10_min": 8.0,
                        "solar_rad": 200.0,
                        "uv_index": 1.5,
                        "rain_size": 1,
                        "rain_rate_last": 0,
                        "rainfall_daily": 0
                    }
                ]
            }
        })";

        auto result = JsonParser::parseCurrentConditions(json);
        QVERIFY(result.iss.has_value());
        QVERIFY(result.bar.has_value());

        // Verify correct field routing regardless of order
        QCOMPARE(result.iss->temperature, 65.0);
        QCOMPARE(result.iss->windDirLast, 270);
        QCOMPARE(result.bar->pressureSeaLevel, 30.01);
        QCOMPARE(result.bar->pressureTrend, 1.0);
    }

    void parseCurrentConditions_allThreeTypes()
    {
        const QByteArray json = R"({
            "data": {
                "conditions": [
                    {
                        "data_structure_type": 1,
                        "temp": 70.0,
                        "hum": 50.0,
                        "dew_point": 49.9,
                        "heat_index": 70.0,
                        "wind_chill": 70.0,
                        "wind_speed_last": 0.0,
                        "wind_dir_last": 0,
                        "wind_speed_hi_last_10_min": 0.0,
                        "solar_rad": 0.0,
                        "uv_index": 0.0,
                        "rain_size": 1,
                        "rain_rate_last": 0,
                        "rainfall_daily": 0
                    },
                    {
                        "data_structure_type": 3,
                        "bar_sea_level": 29.95,
                        "bar_trend": -1
                    },
                    {
                        "data_structure_type": 4,
                        "temp_in": 68.0,
                        "hum_in": 45.0,
                        "dew_point_in": 44.5
                    }
                ]
            }
        })";

        auto result = JsonParser::parseCurrentConditions(json);
        QVERIFY(result.iss.has_value());
        QVERIFY(result.bar.has_value());
        QVERIFY(result.indoor.has_value());

        QCOMPARE(result.iss->temperature, 70.0);
        QCOMPARE(result.bar->pressureSeaLevel, 29.95);
        QCOMPARE(result.bar->pressureTrend, -1.0);
        QCOMPARE(result.indoor->tempIn, 68.0);
        QCOMPARE(result.indoor->humIn, 45.0);
        QCOMPARE(result.indoor->dewPointIn, 44.5);
    }

    void parseCurrentConditions_unknownTypeSilentlySkipped()
    {
        // Type 2 is unknown — should be silently skipped; type 1 still parsed
        const QByteArray json = R"({
            "data": {
                "conditions": [
                    {
                        "data_structure_type": 2,
                        "some_field": 999
                    },
                    {
                        "data_structure_type": 1,
                        "temp": 75.0,
                        "hum": 40.0,
                        "dew_point": 45.0,
                        "heat_index": 75.0,
                        "wind_chill": 75.0,
                        "wind_speed_last": 3.0,
                        "wind_dir_last": 90,
                        "wind_speed_hi_last_10_min": 5.0,
                        "solar_rad": 600.0,
                        "uv_index": 4.0,
                        "rain_size": 1,
                        "rain_rate_last": 0,
                        "rainfall_daily": 0
                    }
                ]
            }
        })";

        auto result = JsonParser::parseCurrentConditions(json);
        QVERIFY(result.iss.has_value());
        QVERIFY(!result.bar.has_value());
        QVERIFY(!result.indoor.has_value());
        QCOMPARE(result.iss->temperature, 75.0);
    }

    void parseCurrentConditions_malformedJson_returnsAllNullopt()
    {
        const QByteArray malformed = "{ not valid json !!!";
        auto result = JsonParser::parseCurrentConditions(malformed);
        QVERIFY(!result.iss.has_value());
        QVERIFY(!result.bar.has_value());
        QVERIFY(!result.indoor.has_value());
    }

    void parseCurrentConditions_emptyConditionsArray_returnsAllNullopt()
    {
        const QByteArray json = R"({"data": {"conditions": []}})";
        auto result = JsonParser::parseCurrentConditions(json);
        QVERIFY(!result.iss.has_value());
        QVERIFY(!result.bar.has_value());
        QVERIFY(!result.indoor.has_value());
    }

    void parseCurrentConditions_emptyJson_returnsAllNullopt()
    {
        auto result = JsonParser::parseCurrentConditions(QByteArray());
        QVERIFY(!result.iss.has_value());
        QVERIFY(!result.bar.has_value());
        QVERIFY(!result.indoor.has_value());
    }

    // -----------------------------------------------------------------------
    // Rain conversion tests via parseCurrentConditions
    // -----------------------------------------------------------------------
    void parseCurrentConditions_rainSize1_convertsCorrectly()
    {
        // 500 counts * 0.01 in/count = 5.0 in/hr
        const QByteArray json = R"({
            "data": {
                "conditions": [
                    {
                        "data_structure_type": 1,
                        "temp": 70.0,
                        "hum": 50.0,
                        "dew_point": 50.0,
                        "heat_index": 70.0,
                        "wind_chill": 70.0,
                        "wind_speed_last": 0.0,
                        "wind_dir_last": 0,
                        "wind_speed_hi_last_10_min": 0.0,
                        "solar_rad": 0.0,
                        "uv_index": 0.0,
                        "rain_size": 1,
                        "rain_rate_last": 500,
                        "rainfall_daily": 100
                    }
                ]
            }
        })";

        auto result = JsonParser::parseCurrentConditions(json);
        QVERIFY(result.iss.has_value());
        QCOMPARE(result.iss->rainRateLast, 500.0 * 0.01);   // 5.0
        QCOMPARE(result.iss->rainfallDaily, 100.0 * 0.01);  // 1.0
    }

    void parseCurrentConditions_rainSize2_convertsCorrectly()
    {
        // 100 counts * (0.2/25.4) in/count
        const QByteArray json = R"({
            "data": {
                "conditions": [
                    {
                        "data_structure_type": 1,
                        "temp": 70.0,
                        "hum": 50.0,
                        "dew_point": 50.0,
                        "heat_index": 70.0,
                        "wind_chill": 70.0,
                        "wind_speed_last": 0.0,
                        "wind_dir_last": 0,
                        "wind_speed_hi_last_10_min": 0.0,
                        "solar_rad": 0.0,
                        "uv_index": 0.0,
                        "rain_size": 2,
                        "rain_rate_last": 100,
                        "rainfall_daily": 50
                    }
                ]
            }
        })";

        const double factor = 0.2 / 25.4;
        auto result = JsonParser::parseCurrentConditions(json);
        QVERIFY(result.iss.has_value());
        QVERIFY(std::fabs(result.iss->rainRateLast - 100.0 * factor) < 1e-10);
        QVERIFY(std::fabs(result.iss->rainfallDaily - 50.0 * factor) < 1e-10);
    }

    void parseCurrentConditions_rainSize3_convertsCorrectly()
    {
        // 200 counts * (0.1/25.4) in/count
        const QByteArray json = R"({
            "data": {
                "conditions": [
                    {
                        "data_structure_type": 1,
                        "temp": 70.0,
                        "hum": 50.0,
                        "dew_point": 50.0,
                        "heat_index": 70.0,
                        "wind_chill": 70.0,
                        "wind_speed_last": 0.0,
                        "wind_dir_last": 0,
                        "wind_speed_hi_last_10_min": 0.0,
                        "solar_rad": 0.0,
                        "uv_index": 0.0,
                        "rain_size": 3,
                        "rain_rate_last": 200,
                        "rainfall_daily": 80
                    }
                ]
            }
        })";

        const double factor = 0.1 / 25.4;
        auto result = JsonParser::parseCurrentConditions(json);
        QVERIFY(result.iss.has_value());
        QVERIFY(std::fabs(result.iss->rainRateLast - 200.0 * factor) < 1e-10);
        QVERIFY(std::fabs(result.iss->rainfallDaily - 80.0 * factor) < 1e-10);
    }

    void parseCurrentConditions_rainSize4_convertsCorrectly()
    {
        // 1000 counts * 0.001 in/count = 1.0 inches
        const QByteArray json = R"({
            "data": {
                "conditions": [
                    {
                        "data_structure_type": 1,
                        "temp": 70.0,
                        "hum": 50.0,
                        "dew_point": 50.0,
                        "heat_index": 70.0,
                        "wind_chill": 70.0,
                        "wind_speed_last": 0.0,
                        "wind_dir_last": 0,
                        "wind_speed_hi_last_10_min": 0.0,
                        "solar_rad": 0.0,
                        "uv_index": 0.0,
                        "rain_size": 4,
                        "rain_rate_last": 1000,
                        "rainfall_daily": 500
                    }
                ]
            }
        })";

        auto result = JsonParser::parseCurrentConditions(json);
        QVERIFY(result.iss.has_value());
        QCOMPARE(result.iss->rainRateLast, 1000.0 * 0.001);  // 1.0
        QCOMPARE(result.iss->rainfallDaily, 500.0 * 0.001);  // 0.5
    }

    // -----------------------------------------------------------------------
    // parseUdpDatagram tests
    // -----------------------------------------------------------------------
    void parseUdpDatagram_validIssData_returnsUdpReading()
    {
        const QByteArray json = R"({
            "conditions": [
                {
                    "data_structure_type": 1,
                    "wind_speed_last": 10.5,
                    "wind_dir_last": 225,
                    "wind_speed_hi_last_10_min": 15.0,
                    "rain_size": 1,
                    "rain_rate_last": 0,
                    "rainfall_daily": 25
                }
            ]
        })";

        auto result = JsonParser::parseUdpDatagram(json);
        QVERIFY(result.has_value());
        QCOMPARE(result->windSpeedLast, 10.5);
        QCOMPARE(result->windDirLast, 225);
        QCOMPARE(result->windSpeedHi10, 15.0);
        QCOMPARE(result->rainSize, 1);
        QCOMPARE(result->rainfallDaily, 25.0 * 0.01);  // converted
    }

    void parseUdpDatagram_malformedJson_returnsNullopt()
    {
        const QByteArray malformed = "{ broken json [";
        auto result = JsonParser::parseUdpDatagram(malformed);
        QVERIFY(!result.has_value());
    }

    void parseUdpDatagram_emptyInput_returnsNullopt()
    {
        auto result = JsonParser::parseUdpDatagram(QByteArray());
        QVERIFY(!result.has_value());
    }

    void parseUdpDatagram_noType1Entry_returnsNullopt()
    {
        // UDP datagram with only type-2 entry (not ISS) — should return nullopt
        const QByteArray json = R"({
            "conditions": [
                {
                    "data_structure_type": 2,
                    "some_field": 123
                }
            ]
        })";

        auto result = JsonParser::parseUdpDatagram(json);
        QVERIFY(!result.has_value());
    }

    void parseUdpDatagram_emptyConditionsArray_returnsNullopt()
    {
        const QByteArray json = R"({"conditions": []})";
        auto result = JsonParser::parseUdpDatagram(json);
        QVERIFY(!result.has_value());
    }

    void parseUdpDatagram_rainConversionApplied()
    {
        // Verify rain counts are converted using rain_size factor
        const QByteArray json = R"({
            "conditions": [
                {
                    "data_structure_type": 1,
                    "wind_speed_last": 5.0,
                    "wind_dir_last": 90,
                    "wind_speed_hi_last_10_min": 7.0,
                    "rain_size": 4,
                    "rain_rate_last": 2000,
                    "rainfall_daily": 1000
                }
            ]
        })";

        auto result = JsonParser::parseUdpDatagram(json);
        QVERIFY(result.has_value());
        QCOMPARE(result->rainRateLast, 2000.0 * 0.001);   // 2.0 in/hr
        QCOMPARE(result->rainfallDaily, 1000.0 * 0.001);  // 1.0 inches
    }

    // -----------------------------------------------------------------------
    // parseForecast helpers
    // -----------------------------------------------------------------------

    static QByteArray makeNwsForecastJson(const QJsonArray &periods) {
        QJsonObject props;
        props["periods"] = periods;
        QJsonObject root;
        root["properties"] = props;
        return QJsonDocument(root).toJson();
    }

    static QJsonObject makePeriod(bool isDaytime, int temp, int precip, const QString &iconUrl) {
        QJsonObject precipObj;
        precipObj["unitCode"] = "wmoUnit:percent";
        precipObj["value"] = precip;
        QJsonObject obj;
        obj["isDaytime"] = isDaytime;
        obj["temperature"] = temp;
        obj["probabilityOfPrecipitation"] = precipObj;
        obj["icon"] = iconUrl;
        return obj;
    }

    static QJsonObject makePeriodNullPrecip(bool isDaytime, int temp, const QString &iconUrl) {
        QJsonObject precipObj;
        precipObj["unitCode"] = "wmoUnit:percent";
        precipObj["value"] = QJsonValue(); // null
        QJsonObject obj;
        obj["isDaytime"] = isDaytime;
        obj["temperature"] = temp;
        obj["probabilityOfPrecipitation"] = precipObj;
        obj["icon"] = iconUrl;
        return obj;
    }

    // -----------------------------------------------------------------------
    // parseForecast tests
    // -----------------------------------------------------------------------

    void parseForecast_morningFetch_returnsThreeDays()
    {
        // 6 periods: day0, night0, day1, night1, day2, night2 — standard morning fetch
        QJsonArray periods;
        periods.append(makePeriod(true,  55, 10, "https://api.weather.gov/icons/land/day/skc?size=medium"));
        periods.append(makePeriod(false, 40,  5, "https://api.weather.gov/icons/land/night/nskc?size=medium"));
        periods.append(makePeriod(true,  60, 20, "https://api.weather.gov/icons/land/day/bkn?size=medium"));
        periods.append(makePeriod(false, 45, 30, "https://api.weather.gov/icons/land/night/rain_showers,40?size=medium"));
        periods.append(makePeriod(true,  65, 15, "https://api.weather.gov/icons/land/day/tsra_hi,40?size=medium"));
        periods.append(makePeriod(false, 50, 25, "https://api.weather.gov/icons/land/night/sn?size=medium"));

        auto result = JsonParser::parseForecast(makeNwsForecastJson(periods));

        QCOMPARE(result.size(), 3);

        // Day 0: high=55, low=40, precip=max(10,5)=10, iconCode="skc"
        QCOMPARE(result[0].high, 55);
        QCOMPARE(result[0].low, 40);
        QCOMPARE(result[0].precip, 10);
        QCOMPARE(result[0].iconCode, QStringLiteral("skc"));

        // Day 1: high=60, low=45, precip=max(20,30)=30, iconCode="bkn"
        QCOMPARE(result[1].high, 60);
        QCOMPARE(result[1].low, 45);
        QCOMPARE(result[1].precip, 30);
        QCOMPARE(result[1].iconCode, QStringLiteral("bkn"));

        // Day 2: high=65, low=50, precip=max(15,25)=25, iconCode="tsra_hi"
        QCOMPARE(result[2].high, 65);
        QCOMPARE(result[2].low, 50);
        QCOMPARE(result[2].precip, 25);
        QCOMPARE(result[2].iconCode, QStringLiteral("tsra_hi"));
    }

    void parseForecast_afternoonFetch_firstPeriodNight_highIsSentinel()
    {
        // First period is nighttime (afternoon fetch edge case)
        // Result[0] should have high=-999, valid low, nighttime icon
        QJsonArray periods;
        periods.append(makePeriod(false, 38,  5, "https://api.weather.gov/icons/land/night/nskc?size=medium"));
        periods.append(makePeriod(true,  58, 20, "https://api.weather.gov/icons/land/day/bkn?size=medium"));
        periods.append(makePeriod(false, 43, 30, "https://api.weather.gov/icons/land/night/rain_showers?size=medium"));
        periods.append(makePeriod(true,  62, 15, "https://api.weather.gov/icons/land/day/tsra?size=medium"));
        periods.append(makePeriod(false, 48, 10, "https://api.weather.gov/icons/land/night/sn?size=medium"));

        auto result = JsonParser::parseForecast(makeNwsForecastJson(periods));

        QCOMPARE(result.size(), 3);

        // Day 0: tonight only — high sentinel, valid low, nighttime icon
        QCOMPARE(result[0].high, -999);
        QCOMPARE(result[0].low, 38);
        QCOMPARE(result[0].precip, 5);
        QCOMPARE(result[0].iconCode, QStringLiteral("nskc"));

        // Day 1: full day — high=58, low=43, precip=max(20,30)=30
        QCOMPARE(result[1].high, 58);
        QCOMPARE(result[1].low, 43);
        QCOMPARE(result[1].precip, 30);
        QCOMPARE(result[1].iconCode, QStringLiteral("bkn"));

        // Day 2: full day — high=62, low=48, precip=max(15,10)=15
        QCOMPARE(result[2].high, 62);
        QCOMPARE(result[2].low, 48);
        QCOMPARE(result[2].precip, 15);
        QCOMPARE(result[2].iconCode, QStringLiteral("tsra"));
    }

    void parseForecast_nullPrecip_treatedAsZero()
    {
        // probabilityOfPrecipitation.value = null -> treated as 0
        QJsonArray periods;
        periods.append(makePeriodNullPrecip(true,  55, "https://api.weather.gov/icons/land/day/skc?size=medium"));
        periods.append(makePeriodNullPrecip(false, 40, "https://api.weather.gov/icons/land/night/nskc?size=medium"));

        auto result = JsonParser::parseForecast(makeNwsForecastJson(periods));

        QCOMPARE(result.size(), 1);
        QCOMPARE(result[0].precip, 0); // null treated as 0
    }

    void parseForecast_emptyOrMalformed_returnsEmpty()
    {
        // Empty string
        QVERIFY(JsonParser::parseForecast(QByteArray()).isEmpty());
        // Invalid JSON
        QVERIFY(JsonParser::parseForecast("{ not json").isEmpty());
        // Missing properties
        QVERIFY(JsonParser::parseForecast("{}").isEmpty());
        // Empty periods array
        QJsonArray emptyPeriods;
        QVERIFY(JsonParser::parseForecast(makeNwsForecastJson(emptyPeriods)).isEmpty());
    }

    void parseForecast_precipMax_usesMaxOfDayAndNight()
    {
        // Day precip=10, night precip=40 -> result precip=40
        QJsonArray periods;
        periods.append(makePeriod(true,  55, 10, "https://api.weather.gov/icons/land/day/skc?size=medium"));
        periods.append(makePeriod(false, 40, 40, "https://api.weather.gov/icons/land/night/rain_showers?size=medium"));

        auto result = JsonParser::parseForecast(makeNwsForecastJson(periods));

        QCOMPARE(result.size(), 1);
        QCOMPARE(result[0].precip, 40);
    }

    void parseForecast_partialData_fourPeriods_returnsTwoDays()
    {
        // Only 4 periods (2 full days) — returns 2 items, not 3
        QJsonArray periods;
        periods.append(makePeriod(true,  55, 10, "https://api.weather.gov/icons/land/day/skc?size=medium"));
        periods.append(makePeriod(false, 40,  5, "https://api.weather.gov/icons/land/night/nskc?size=medium"));
        periods.append(makePeriod(true,  60, 20, "https://api.weather.gov/icons/land/day/bkn?size=medium"));
        periods.append(makePeriod(false, 45, 30, "https://api.weather.gov/icons/land/night/rain_showers?size=medium"));

        auto result = JsonParser::parseForecast(makeNwsForecastJson(periods));

        QCOMPARE(result.size(), 2);
    }

    void parseForecast_iconCodeExtraction_stripsProb_andQueryString()
    {
        // URL with probability suffix ,40 AND query string ?size=medium
        // "https://api.weather.gov/icons/land/night/rain_showers,40?size=medium" -> "rain_showers"
        QJsonArray periods;
        periods.append(makePeriod(true,  55, 10, "https://api.weather.gov/icons/land/day/tsra_hi,40?size=medium"));
        periods.append(makePeriod(false, 40, 20, "https://api.weather.gov/icons/land/night/rain_showers,60?size=medium"));

        auto result = JsonParser::parseForecast(makeNwsForecastJson(periods));

        QCOMPARE(result.size(), 1);
        QCOMPARE(result[0].iconCode, QStringLiteral("tsra_hi"));
    }
};

QTEST_MAIN(TstJsonParser)
#include "tst_JsonParser.moc"
