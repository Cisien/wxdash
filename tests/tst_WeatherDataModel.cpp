#include "models/WeatherDataModel.h"
#include "models/WeatherReadings.h"

#include <QSignalSpy>
#include <QtTest/QtTest>

class TstWeatherDataModel : public QObject {
    Q_OBJECT

private:
    // Helper: create an IssReading with all zeros except specified fields
    static IssReading makeIss(double temp = 0.0, double hum = 0.0, double dewPoint = 0.0,
                              double heatIndex = 0.0, double windChill = 0.0,
                              double windSpeed = 0.0, int windDir = 0, double windGust = 0.0,
                              double solarRad = 0.0, double uvIndex = 0.0, double rainRate = 0.0,
                              double rainfallDaily = 0.0, int rainSize = 1) {
        IssReading r;
        r.temperature = temp;
        r.humidity = hum;
        r.dewPoint = dewPoint;
        r.heatIndex = heatIndex;
        r.windChill = windChill;
        r.windSpeedLast = windSpeed;
        r.windDirLast = windDir;
        r.windSpeedHi10 = windGust;
        r.solarRad = solarRad;
        r.uvIndex = uvIndex;
        r.rainRateLast = rainRate;
        r.rainfallDaily = rainfallDaily;
        r.rainSize = rainSize;
        return r;
    }

    // Helper: make a fake clock that returns a controllable value
    static std::function<qint64()> makeClock(qint64* msPtr) {
        return [msPtr]() -> qint64 { return *msPtr; };
    }

private slots:
    // -----------------------------------------------------------------------
    // applyIssUpdate tests
    // -----------------------------------------------------------------------

    void applyIssUpdate_emitsTemperatureChanged() {
        qint64 fakeMs = 0;
        WeatherDataModel model(nullptr, makeClock(&fakeMs));
        QSignalSpy spy(&model, &WeatherDataModel::temperatureChanged);

        IssReading r = makeIss(/*temp=*/72.5);
        model.applyIssUpdate(r);

        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toDouble(), 72.5);
        QCOMPARE(model.temperature(), 72.5);
    }

    void applyIssUpdate_sameValue_noSignal() {
        qint64 fakeMs = 0;
        WeatherDataModel model(nullptr, makeClock(&fakeMs));

        // Apply once to set the value
        model.applyIssUpdate(makeIss(/*temp=*/72.5));

        QSignalSpy spy(&model, &WeatherDataModel::temperatureChanged);

        // Apply same value again — no signal should fire
        model.applyIssUpdate(makeIss(/*temp=*/72.5));
        QCOMPARE(spy.count(), 0);
    }

    void applyIssUpdate_setsAllIssFields() {
        qint64 fakeMs = 0;
        WeatherDataModel model(nullptr, makeClock(&fakeMs));

        IssReading r;
        r.temperature = 72.5;
        r.humidity = 55.0;
        r.dewPoint = 55.3;
        r.heatIndex = 73.1;
        r.windChill = 72.5;
        r.windSpeedLast = 8.0;
        r.windDirLast = 180;
        r.windSpeedHi10 = 12.0;
        r.solarRad = 450.0;
        r.uvIndex = 3.2;
        r.rainRateLast = 0.5;
        r.rainfallDaily = 1.0;
        r.rainSize = 1;
        model.applyIssUpdate(r);

        QCOMPARE(model.temperature(), 72.5);
        QCOMPARE(model.humidity(), 55.0);
        QCOMPARE(model.dewPoint(), 55.3);
        QCOMPARE(model.heatIndex(), 73.1);
        QCOMPARE(model.windChill(), 72.5);
        QCOMPARE(model.windSpeed(), 8.0);
        QCOMPARE(model.windDir(), 180);
        QCOMPARE(model.windGust(), 12.0);
        QCOMPARE(model.solarRad(), 450.0);
        QCOMPARE(model.uvIndex(), 3.2);
        QCOMPARE(model.rainRate(), 0.5);
        QCOMPARE(model.rainfallDaily(), 1.0);
    }

    // -----------------------------------------------------------------------
    // applyBarUpdate tests
    // -----------------------------------------------------------------------

    void applyBarUpdate_emitsPressureChanged() {
        qint64 fakeMs = 0;
        WeatherDataModel model(nullptr, makeClock(&fakeMs));
        QSignalSpy spyPressure(&model, &WeatherDataModel::pressureChanged);
        QSignalSpy spyTrend(&model, &WeatherDataModel::pressureTrendChanged);

        BarReading r;
        r.pressureSeaLevel = 29.92;
        r.pressureTrend = 1;
        model.applyBarUpdate(r);

        QCOMPARE(spyPressure.count(), 1);
        QCOMPARE(spyPressure.at(0).at(0).toDouble(), 29.92);
        QCOMPARE(spyTrend.count(), 1);
        QCOMPARE(spyTrend.at(0).at(0).toInt(), 1);
        QCOMPARE(model.pressure(), 29.92);
        QCOMPARE(model.pressureTrend(), 1);
    }

    void applyBarUpdate_sameValue_noSignal() {
        qint64 fakeMs = 0;
        WeatherDataModel model(nullptr, makeClock(&fakeMs));

        BarReading r;
        r.pressureSeaLevel = 29.92;
        r.pressureTrend = 0;
        model.applyBarUpdate(r);

        QSignalSpy spy(&model, &WeatherDataModel::pressureChanged);
        model.applyBarUpdate(r);
        QCOMPARE(spy.count(), 0);
    }

    // -----------------------------------------------------------------------
    // applyIndoorUpdate tests
    // -----------------------------------------------------------------------

    void applyIndoorUpdate_emitsTempInChanged() {
        qint64 fakeMs = 0;
        WeatherDataModel model(nullptr, makeClock(&fakeMs));
        QSignalSpy spy(&model, &WeatherDataModel::tempInChanged);

        IndoorReading r;
        r.tempIn = 68.0;
        r.humIn = 45.0;
        r.dewPointIn = 44.5;
        model.applyIndoorUpdate(r);

        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toDouble(), 68.0);
        QCOMPARE(model.tempIn(), 68.0);
        QCOMPARE(model.humIn(), 45.0);
        QCOMPARE(model.dewPointIn(), 44.5);
    }

    void applyIndoorUpdate_sameValue_noSignal() {
        qint64 fakeMs = 0;
        WeatherDataModel model(nullptr, makeClock(&fakeMs));

        IndoorReading r;
        r.tempIn = 68.0;
        r.humIn = 45.0;
        r.dewPointIn = 44.5;
        model.applyIndoorUpdate(r);

        QSignalSpy spy(&model, &WeatherDataModel::tempInChanged);
        model.applyIndoorUpdate(r);
        QCOMPARE(spy.count(), 0);
    }

    // -----------------------------------------------------------------------
    // applyUdpUpdate tests
    // -----------------------------------------------------------------------

    void applyUdpUpdate_overwritesWindSpeed() {
        qint64 fakeMs = 0;
        WeatherDataModel model(nullptr, makeClock(&fakeMs));

        // First set via ISS HTTP
        model.applyIssUpdate(makeIss(/*temp=*/70.0, 0, 0, 0, 0, /*windSpeed=*/5.0));

        QSignalSpy spy(&model, &WeatherDataModel::windSpeedChanged);

        // Then override with UDP
        UdpReading udp;
        udp.windSpeedLast = 8.0;
        udp.windDirLast = 180;
        udp.windSpeedHi10 = 10.0;
        udp.rainRateLast = 0.0;
        udp.rainfallDaily = 0.0;
        udp.rainSize = 1;
        model.applyUdpUpdate(udp);

        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toDouble(), 8.0);
        QCOMPARE(model.windSpeed(), 8.0);
    }

    void applyUdpUpdate_setsAllUdpFields() {
        qint64 fakeMs = 0;
        WeatherDataModel model(nullptr, makeClock(&fakeMs));

        UdpReading udp;
        udp.windSpeedLast = 12.5;
        udp.windDirLast = 270;
        udp.windSpeedHi10 = 18.0;
        udp.rainRateLast = 1.5;
        udp.rainfallDaily = 0.25;
        udp.rainSize = 1;
        model.applyUdpUpdate(udp);

        QCOMPARE(model.windSpeed(), 12.5);
        QCOMPARE(model.windDir(), 270);
        QCOMPARE(model.windGust(), 18.0);
        QCOMPARE(model.rainRate(), 1.5);
        QCOMPARE(model.rainfallDaily(), 0.25);
    }

    // -----------------------------------------------------------------------
    // Staleness tests (injectable clock — no real 30s wait)
    // -----------------------------------------------------------------------

    void staleness_emitsSignalAfter30s() {
        qint64 fakeMs = 0;
        WeatherDataModel model(nullptr, makeClock(&fakeMs));
        QSignalSpy spy(&model, &WeatherDataModel::sourceStaleChanged);

        // Apply an update to start the staleness clock
        model.applyIssUpdate(makeIss(72.5));

        // Advance fake clock past staleness threshold
        fakeMs = 31000;
        model.checkStaleness();

        QCOMPARE(spy.count(), 1);
        QVERIFY(spy.at(0).at(0).toBool() == true);
        QVERIFY(model.sourceStale());
    }

    void staleness_clearsAllValues() {
        qint64 fakeMs = 0;
        WeatherDataModel model(nullptr, makeClock(&fakeMs));

        // Set various values
        IssReading iss;
        iss.temperature = 72.5;
        iss.humidity = 55.0;
        iss.dewPoint = 55.3;
        iss.heatIndex = 73.1;
        iss.windChill = 72.5;
        iss.windSpeedLast = 8.0;
        iss.windDirLast = 180;
        iss.windSpeedHi10 = 12.0;
        iss.solarRad = 450.0;
        iss.uvIndex = 3.2;
        iss.rainRateLast = 0.5;
        iss.rainfallDaily = 1.0;
        model.applyIssUpdate(iss);

        BarReading bar;
        bar.pressureSeaLevel = 29.92;
        bar.pressureTrend = 1;
        model.applyBarUpdate(bar);

        IndoorReading indoor;
        indoor.tempIn = 68.0;
        indoor.humIn = 45.0;
        indoor.dewPointIn = 44.5;
        model.applyIndoorUpdate(indoor);

        // Trigger staleness
        fakeMs = 31000;
        model.checkStaleness();

        // All values should be cleared to 0
        QCOMPARE(model.temperature(), 0.0);
        QCOMPARE(model.humidity(), 0.0);
        QCOMPARE(model.dewPoint(), 0.0);
        QCOMPARE(model.heatIndex(), 0.0);
        QCOMPARE(model.windChill(), 0.0);
        QCOMPARE(model.windSpeed(), 0.0);
        QCOMPARE(model.windDir(), 0);
        QCOMPARE(model.windGust(), 0.0);
        QCOMPARE(model.solarRad(), 0.0);
        QCOMPARE(model.uvIndex(), 0.0);
        QCOMPARE(model.rainRate(), 0.0);
        QCOMPARE(model.rainfallDaily(), 0.0);
        QCOMPARE(model.pressure(), 0.0);
        QCOMPARE(model.pressureTrend(), 0);
        QCOMPARE(model.tempIn(), 0.0);
        QCOMPARE(model.humIn(), 0.0);
        QCOMPARE(model.dewPointIn(), 0.0);
    }

    void staleness_clearsEmitsSignals() {
        qint64 fakeMs = 0;
        WeatherDataModel model(nullptr, makeClock(&fakeMs));

        // Set values so all changed signals will fire when cleared to 0
        IssReading iss;
        iss.temperature = 72.5;
        iss.humidity = 55.0;
        iss.dewPoint = 55.3;
        iss.heatIndex = 73.1;
        iss.windChill = 72.5;
        iss.windSpeedLast = 8.0;
        iss.windDirLast = 180;
        iss.windSpeedHi10 = 12.0;
        iss.solarRad = 450.0;
        iss.uvIndex = 3.2;
        iss.rainRateLast = 0.5;
        iss.rainfallDaily = 1.0;
        model.applyIssUpdate(iss);

        // Track all changed signals
        QSignalSpy spyTemp(&model, &WeatherDataModel::temperatureChanged);
        QSignalSpy spyHum(&model, &WeatherDataModel::humidityChanged);
        QSignalSpy spyWind(&model, &WeatherDataModel::windSpeedChanged);
        QSignalSpy spySolar(&model, &WeatherDataModel::solarRadChanged);

        fakeMs = 31000;
        model.checkStaleness();

        // All per-field changed signals should have fired on clearing
        QVERIFY(spyTemp.count() >= 1);
        QVERIFY(spyHum.count() >= 1);
        QVERIFY(spyWind.count() >= 1);
        QVERIFY(spySolar.count() >= 1);
    }

    void recovery_afterStaleness() {
        qint64 fakeMs = 0;
        WeatherDataModel model(nullptr, makeClock(&fakeMs));

        // Apply initial data and go stale
        model.applyIssUpdate(makeIss(72.5));
        fakeMs = 31000;
        model.checkStaleness();
        QVERIFY(model.sourceStale());

        // Now track sourceStaleChanged for recovery
        QSignalSpy spy(&model, &WeatherDataModel::sourceStaleChanged);

        // Reset fake clock back to a fresh time (simulate new update arriving)
        fakeMs = 0;
        model.applyIssUpdate(makeIss(75.0));

        // sourceStaleChanged(false) should have been emitted
        QCOMPARE(spy.count(), 1);
        QVERIFY(spy.at(0).at(0).toBool() == false);
        QVERIFY(!model.sourceStale());
    }

    void noFalsePositiveAtStartup() {
        // Create model, call checkStaleness immediately — no update has been received
        // Should NOT emit sourceStaleChanged because the timer was never started
        qint64 fakeMs = 0;
        WeatherDataModel model(nullptr, makeClock(&fakeMs));
        QSignalSpy spy(&model, &WeatherDataModel::sourceStaleChanged);

        model.checkStaleness();

        QCOMPARE(spy.count(), 0);
        QVERIFY(!model.sourceStale());
    }

    void udpUpdate_resetsStalenessClock() {
        // Apply ISS update, advance clock to 20s (not yet stale),
        // then apply UDP update (clock reset), advance to 20s more,
        // verify no staleness (would have been stale after 30s total without reset)
        qint64 fakeMs = 0;
        WeatherDataModel model(nullptr, makeClock(&fakeMs));
        QSignalSpy spy(&model, &WeatherDataModel::sourceStaleChanged);

        // Apply ISS first to start timer
        model.applyIssUpdate(makeIss(72.5));

        // Advance to 20s (not yet stale)
        fakeMs = 20000;

        // Apply UDP update — this should reset the internal timer reference
        UdpReading udp;
        udp.windSpeedLast = 5.0;
        udp.windDirLast = 90;
        udp.windSpeedHi10 = 7.0;
        udp.rainRateLast = 0.0;
        udp.rainfallDaily = 0.0;
        udp.rainSize = 1;
        model.applyUdpUpdate(udp);

        // The model internally records the current clock value as the last-update time
        // So we reset fakeMs to simulate 20s since the UDP update
        // (total 40s since ISS, but only 20s since UDP — should NOT be stale)
        fakeMs =
            20000; // 20s since UDP update (clock is now at 40s total, but last update was at 20s)

        // Need to think about this differently:
        // The model stores the elapsed value at time of last update.
        // checkStaleness computes: currentElapsed - lastUpdateElapsed > 30000
        // So after UDP at fakeMs=20000, lastUpdateElapsed=20000
        // Now if fakeMs=45000 (25s since UDP), should NOT be stale
        fakeMs = 45000;
        model.checkStaleness();
        QCOMPARE(spy.count(), 0);

        // But at fakeMs=55000 (35s since UDP), SHOULD be stale
        fakeMs = 55000;
        model.checkStaleness();
        QCOMPARE(spy.count(), 1);
        QVERIFY(spy.at(0).at(0).toBool() == true);
    }

    void staleness_noDoubleEmit() {
        // Once stale, calling checkStaleness again should NOT re-emit
        qint64 fakeMs = 0;
        WeatherDataModel model(nullptr, makeClock(&fakeMs));

        model.applyIssUpdate(makeIss(72.5));
        fakeMs = 31000;
        model.checkStaleness();

        QSignalSpy spy(&model, &WeatherDataModel::sourceStaleChanged);
        // Second check — already stale, no re-emit
        fakeMs = 40000;
        model.checkStaleness();
        QCOMPARE(spy.count(), 0);
    }

    // -----------------------------------------------------------------------
    // Sparkline ring buffer tests
    // -----------------------------------------------------------------------

    void sparklineInitiallyEmpty()
    {
        WeatherDataModel model;
        QVERIFY(model.temperatureHistory().isEmpty());
        QVERIFY(model.pressureHistory().isEmpty());
    }

    void sparklineRecordsSamples()
    {
        WeatherDataModel model;
        IssReading r = makeIss(/*temp=*/72.0);
        model.applyIssUpdate(r);
        r = makeIss(/*temp=*/73.0);
        model.applyIssUpdate(r);
        r = makeIss(/*temp=*/74.0);
        model.applyIssUpdate(r);

        QVariantList hist = model.temperatureHistory();
        QCOMPARE(hist.size(), 3);
        QCOMPARE(hist[0].toDouble(), 72.0);
        QCOMPARE(hist[1].toDouble(), 73.0);
        QCOMPARE(hist[2].toDouble(), 74.0);
    }

    void sparklineWrapsAtCapacity()
    {
        WeatherDataModel model;
        // Fill to capacity + 10
        for (int i = 0; i < WeatherDataModel::kSparklineCapacity + 10; i++) {
            IssReading r = makeIss(/*temp=*/static_cast<double>(i));
            model.applyIssUpdate(r);
        }
        QVariantList hist = model.temperatureHistory();
        QCOMPARE(hist.size(), WeatherDataModel::kSparklineCapacity);
        // First element should be sample 10 (first 10 evicted)
        QCOMPARE(hist[0].toDouble(), 10.0);
        // Last element should be the most recent
        QCOMPARE(hist[hist.size() - 1].toDouble(),
                 static_cast<double>(WeatherDataModel::kSparklineCapacity + 9));
    }

    void pressureSparklineFromBarUpdate()
    {
        WeatherDataModel model;
        BarReading br;
        br.pressureSeaLevel = 30.10;
        model.applyBarUpdate(br);
        br.pressureSeaLevel = 30.15;
        model.applyBarUpdate(br);

        QVariantList hist = model.pressureHistory();
        QCOMPARE(hist.size(), 2);
        QCOMPARE(hist[0].toDouble(), 30.10);
        QCOMPARE(hist[1].toDouble(), 30.15);
    }
};

QTEST_MAIN(TstWeatherDataModel)
#include "tst_WeatherDataModel.moc"
