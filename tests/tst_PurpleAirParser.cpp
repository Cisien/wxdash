#include <QTest>
#include "network/JsonParser.h"

class tst_PurpleAirParser : public QObject {
    Q_OBJECT
private slots:
    void aqiBoundary_data();
    void aqiBoundary();
    void aqiAboveRange();
    void aqiNegative();
    void parseValidJson();
    void parseMissingChannelB();
    void parseMalformedJson();
    void parsePm10();
};

void tst_PurpleAirParser::aqiBoundary_data()
{
    QTest::addColumn<double>("pm25");
    QTest::addColumn<int>("expectedAqi");

    QTest::newRow("good-low")          << 0.0   << 0;
    QTest::newRow("good-high")         << 9.0   << 50;
    QTest::newRow("moderate-low")      << 9.1   << 51;
    QTest::newRow("moderate-high")     << 35.4  << 100;
    QTest::newRow("usg-low")           << 35.5  << 101;
    QTest::newRow("usg-high")          << 55.4  << 150;
    QTest::newRow("unhealthy-low")     << 55.5  << 151;
    QTest::newRow("unhealthy-high")    << 150.4 << 200;
    QTest::newRow("veryunhealthy-low") << 150.5 << 201;
    QTest::newRow("veryunhealthy-high")<< 250.4 << 300;
    QTest::newRow("hazardous-low")     << 250.5 << 301;
    QTest::newRow("hazardous-high")    << 500.4 << 500;
}

void tst_PurpleAirParser::aqiBoundary()
{
    QFETCH(double, pm25);
    QFETCH(int, expectedAqi);
    QCOMPARE(JsonParser::calculateAqi(pm25), expectedAqi);
}

void tst_PurpleAirParser::aqiAboveRange()
{
    QCOMPARE(JsonParser::calculateAqi(600.0), 500);
}

void tst_PurpleAirParser::aqiNegative()
{
    QCOMPARE(JsonParser::calculateAqi(-5.0), 0);
}

void tst_PurpleAirParser::parseValidJson()
{
    QByteArray json = R"({"pm2_5_atm":12.5,"pm2_5_atm_b":13.5,"pm10_0_atm":20.0})";
    auto r = JsonParser::parsePurpleAirJson(json);
    QCOMPARE(r.pm25_a, 12.5);
    QCOMPARE(r.pm25_b, 13.5);
    QCOMPARE(r.pm25avg, 13.0);  // (12.5 + 13.5) / 2
    QCOMPARE(r.pm10, 20.0);
    QVERIFY(r.aqi > 0);         // 13.0 PM2.5 -> Moderate range
}

void tst_PurpleAirParser::parseMissingChannelB()
{
    QByteArray json = R"({"pm2_5_atm":10.0,"pm10_0_atm":15.0})";
    auto r = JsonParser::parsePurpleAirJson(json);
    QCOMPARE(r.pm25_a, 10.0);
    QCOMPARE(r.pm25_b, 0.0);    // missing -> 0.0 default
    QCOMPARE(r.pm25avg, 5.0);   // (10.0 + 0.0) / 2
    QCOMPARE(r.pm10, 15.0);
}

void tst_PurpleAirParser::parseMalformedJson()
{
    QByteArray json = "not json at all";
    auto r = JsonParser::parsePurpleAirJson(json);
    QCOMPARE(r.pm25_a, 0.0);
    QCOMPARE(r.pm25_b, 0.0);
    QCOMPARE(r.pm25avg, 0.0);
    QCOMPARE(r.pm10, 0.0);
    QCOMPARE(r.aqi, 0.0);
}

void tst_PurpleAirParser::parsePm10()
{
    QByteArray json = R"({"pm2_5_atm":5.0,"pm2_5_atm_b":5.0,"pm10_0_atm":30.0})";
    auto r = JsonParser::parsePurpleAirJson(json);
    QCOMPARE(r.pm10, 30.0);
}

QTEST_MAIN(tst_PurpleAirParser)
#include "tst_PurpleAirParser.moc"
