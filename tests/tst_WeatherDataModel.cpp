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
        r.pressureTrend = 0.034;
        model.applyBarUpdate(r);

        QCOMPARE(spyPressure.count(), 1);
        QCOMPARE(spyPressure.at(0).at(0).toDouble(), 29.92);
        QCOMPARE(spyTrend.count(), 1);
        QCOMPARE(spyTrend.at(0).at(0).toDouble(), 0.034);
        QCOMPARE(model.pressure(), 29.92);
        QCOMPARE(model.pressureTrend(), 0.034);
    }

    void applyBarUpdate_sameValue_noSignal() {
        qint64 fakeMs = 0;
        WeatherDataModel model(nullptr, makeClock(&fakeMs));

        BarReading r;
        r.pressureSeaLevel = 29.92;
        r.pressureTrend = 0.0;
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
        bar.pressureTrend = 0.034;
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
        QCOMPARE(model.pressureTrend(), 0.0);
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
        // History is pre-decimated (stride = capacity / kMaxSparklinePoints)
        QVERIFY(hist.size() <= WeatherDataModel::kMaxSparklinePoints + 10);
        QVERIFY(hist.size() > 0);
        // First element should be from the oldest retained samples (first 10 evicted)
        QVERIFY(hist[0].toDouble() >= 10.0);
        // Last element should be near the most recent (stride may skip the exact last sample)
        double lastVal = hist[hist.size() - 1].toDouble();
        double expected = static_cast<double>(WeatherDataModel::kSparklineCapacity + 9);
        QVERIFY(lastVal >= expected - 20.0);
        QVERIFY(lastVal <= expected);
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

    // -----------------------------------------------------------------------
    // Wind rose: calm sample tracking (WIND-02)
    // -----------------------------------------------------------------------

    void windRose_calmSamplesNoBinCount()
    {
        // Calm readings (dir=0, speed=0) should not appear in any directional bin
        qint64 fakeMs = 0;
        WeatherDataModel model(nullptr, makeClock(&fakeMs));

        // Feed 50 calm readings
        UdpReading calm;
        calm.windSpeedLast = 0.0;
        calm.windDirLast = 0;
        calm.windSpeedHi10 = 0.0;
        calm.rainRateLast = 0.0;
        calm.rainfallDaily = 0.0;
        calm.rainSize = 1;
        for (int i = 0; i < 50; i++)
            model.applyUdpUpdate(calm);

        QCOMPARE(model.windRoseMaxCount(), 0);
        QVariantList data = model.windRoseData();
        QCOMPARE(data.size(), 16);
        for (int i = 0; i < 16; i++) {
            QVariantMap bin = data[i].toMap();
            QCOMPARE(bin[QStringLiteral("count")].toInt(), 0);
        }
    }

    void windRose_calmSamplesOccupyRingSlots()
    {
        // Calm samples occupy ring buffer slots and participate in eviction.
        // Verify indirectly: fill 710 calm + 10 directional = 720 (full).
        // Then feed 1 more calm => oldest calm evicted.
        // Then feed 1 more directional => next-oldest calm evicted (not a directional one).
        // All 10+1=11 directional samples should remain in bin counts.
        qint64 fakeMs = 0;
        WeatherDataModel model(nullptr, makeClock(&fakeMs));

        UdpReading calm;
        calm.windSpeedLast = 0.0;
        calm.windDirLast = 0;
        calm.windSpeedHi10 = 0.0;
        calm.rainRateLast = 0.0;
        calm.rainfallDaily = 0.0;
        calm.rainSize = 1;

        // 710 calm readings
        for (int i = 0; i < 710; i++)
            model.applyUdpUpdate(calm);

        // 10 directional readings (East = 90 degrees, bin 4)
        UdpReading east;
        east.windSpeedLast = 5.0;
        east.windDirLast = 90;
        east.windSpeedHi10 = 5.0;
        east.rainRateLast = 0.0;
        east.rainfallDaily = 0.0;
        east.rainSize = 1;
        for (int i = 0; i < 10; i++)
            model.applyUdpUpdate(east);

        // Buffer full at 720. Now feed 1 more calm => evicts oldest calm (slot 0)
        model.applyUdpUpdate(calm);
        // Feed 1 directional => evicts next-oldest calm (slot 1), NOT a directional
        model.applyUdpUpdate(east);

        // All 11 directional samples should remain (10 original + 1 new)
        QVariantList data = model.windRoseData();
        int eastBin = 4; // 90/22.5 = 4
        QVariantMap eastData = data[eastBin].toMap();
        QCOMPARE(eastData[QStringLiteral("count")].toInt(), 11);
    }

    void windRose_recentAvgSpeedField()
    {
        // windRoseData entries must have a recentAvgSpeed field
        qint64 fakeMs = 0;
        WeatherDataModel model(nullptr, makeClock(&fakeMs));

        UdpReading udp;
        udp.windSpeedLast = 10.0;
        udp.windDirLast = 180; // South, bin 8
        udp.windSpeedHi10 = 10.0;
        udp.rainRateLast = 0.0;
        udp.rainfallDaily = 0.0;
        udp.rainSize = 1;
        model.applyUdpUpdate(udp);

        QVariantList data = model.windRoseData();
        QVariantMap bin = data[8].toMap(); // 180/22.5 = 8
        QVERIFY(bin.contains(QStringLiteral("recentAvgSpeed")));
        QCOMPARE(bin[QStringLiteral("recentAvgSpeed")].toDouble(), 10.0);
    }

    void windRose_recentAvgSpeedUsesLast24Samples()
    {
        // Feed 30 samples into the same bin: first 20 at speed=5, then 10 at speed=20.
        // recentAvgSpeed (last 24 samples) = (14*5 + 10*20)/24 = 11.25
        // Full-window avgSpeed = (20*5 + 10*20)/30 = 10.0
        qint64 fakeMs = 0;
        WeatherDataModel model(nullptr, makeClock(&fakeMs));

        UdpReading udp;
        udp.windDirLast = 90; // East, bin 4
        udp.windSpeedHi10 = 0.0;
        udp.rainRateLast = 0.0;
        udp.rainfallDaily = 0.0;
        udp.rainSize = 1;

        // First 20 at speed=5
        udp.windSpeedLast = 5.0;
        for (int i = 0; i < 20; i++)
            model.applyUdpUpdate(udp);

        // Then 10 at speed=20
        udp.windSpeedLast = 20.0;
        for (int i = 0; i < 10; i++)
            model.applyUdpUpdate(udp);

        QVariantList data = model.windRoseData();
        QVariantMap bin = data[4].toMap(); // bin 4 = East

        // Full-window average should be (20*5 + 10*20)/30 = 10.0
        double fullAvg = bin[QStringLiteral("avgSpeed")].toDouble();
        QVERIFY(qAbs(fullAvg - 10.0) < 0.01);

        // Recent average (last 24): 14 samples at 5 + 10 samples at 20 = (70+200)/24 = 11.25
        double recentAvg = bin[QStringLiteral("recentAvgSpeed")].toDouble();
        QVERIFY(qAbs(recentAvg - 11.25) < 0.01);
    }

    void windRose_recentAvgSpeedFewerThan24()
    {
        // When a bin has fewer than 24 samples, recentAvgSpeed averages all samples
        qint64 fakeMs = 0;
        WeatherDataModel model(nullptr, makeClock(&fakeMs));

        UdpReading udp;
        udp.windDirLast = 270; // West, bin 12
        udp.windSpeedLast = 10.0;
        udp.windSpeedHi10 = 0.0;
        udp.rainRateLast = 0.0;
        udp.rainfallDaily = 0.0;
        udp.rainSize = 1;

        for (int i = 0; i < 5; i++)
            model.applyUdpUpdate(udp);

        QVariantList data = model.windRoseData();
        QVariantMap bin = data[12].toMap();
        QCOMPARE(bin[QStringLiteral("recentAvgSpeed")].toDouble(), 10.0);
    }

    void windRose_mixedCalmAndDirectional()
    {
        // Alternating calm and directional readings:
        // calm samples occupy ring slots, only directional appear in bins
        qint64 fakeMs = 0;
        WeatherDataModel model(nullptr, makeClock(&fakeMs));

        UdpReading calm;
        calm.windSpeedLast = 0.0;
        calm.windDirLast = 0;
        calm.windSpeedHi10 = 0.0;
        calm.rainRateLast = 0.0;
        calm.rainfallDaily = 0.0;
        calm.rainSize = 1;

        UdpReading north;
        north.windSpeedLast = 10.0;
        north.windDirLast = 360; // True north wraps to bin 0 (360/22.5 = 16 % 16 = 0)
        north.windSpeedHi10 = 10.0;
        north.rainRateLast = 0.0;
        north.rainfallDaily = 0.0;
        north.rainSize = 1;

        // Alternate: calm, north, calm, north, ... (20 each = 40 total)
        for (int i = 0; i < 20; i++) {
            model.applyUdpUpdate(calm);
            model.applyUdpUpdate(north);
        }

        // 20 directional samples in bin 0 (North/360 degrees)
        QVariantList data = model.windRoseData();
        QVariantMap northBin = data[0].toMap();
        QCOMPARE(northBin[QStringLiteral("count")].toInt(), 20);
        QCOMPARE(northBin[QStringLiteral("avgSpeed")].toDouble(), 10.0);

        // All other bins should be empty
        for (int i = 1; i < 16; i++) {
            QVariantMap bin = data[i].toMap();
            QCOMPARE(bin[QStringLiteral("count")].toInt(), 0);
        }

        // windRoseMaxCount should reflect only directional samples
        QCOMPARE(model.windRoseMaxCount(), 20);
    }

    // -----------------------------------------------------------------------
    // True min/max tracking tests
    // -----------------------------------------------------------------------

    void trueMinMax_initiallyZero()
    {
        WeatherDataModel model;
        QCOMPARE(model.temperatureMin(), 0.0);
        QCOMPARE(model.temperatureMax(), 0.0);
        QCOMPARE(model.windSpeedMin(), 0.0);
        QCOMPARE(model.windSpeedMax(), 0.0);
    }

    void trueMinMax_tracksExtremes()
    {
        WeatherDataModel model;
        model.applyIssUpdate(makeIss(/*temp=*/72.0));
        QCOMPARE(model.temperatureMin(), 72.0);
        QCOMPARE(model.temperatureMax(), 72.0);

        model.applyIssUpdate(makeIss(/*temp=*/65.0));
        QCOMPARE(model.temperatureMin(), 65.0);
        QCOMPARE(model.temperatureMax(), 72.0);

        model.applyIssUpdate(makeIss(/*temp=*/80.0));
        QCOMPARE(model.temperatureMin(), 65.0);
        QCOMPARE(model.temperatureMax(), 80.0);
    }

    void trueMinMax_survivesDecimation()
    {
        // Fill buffer well past kMaxSparklinePoints so stride > 1.
        // The true min/max must still reflect the exact extremes,
        // not just the strided samples.
        WeatherDataModel model;

        // Insert 2000 values in the range [10..30]
        for (int i = 0; i < 2000; i++) {
            double temp = 20.0 + 10.0 * std::sin(i * 0.01);
            model.applyIssUpdate(makeIss(temp));
        }

        // Insert one spike at 99.0 (must be captured as trueMax)
        model.applyIssUpdate(makeIss(99.0));

        // Insert one dip at -5.0 (must be captured as trueMin)
        model.applyIssUpdate(makeIss(-5.0));

        // Continue with normal values
        for (int i = 0; i < 500; i++)
            model.applyIssUpdate(makeIss(20.0));

        // Verify the cache is decimated (stride > 1)
        QVariantList hist = model.temperatureHistory();
        QVERIFY(hist.size() < 2503);

        // True min/max must capture the exact extremes
        QCOMPARE(model.temperatureMin(), -5.0);
        QCOMPARE(model.temperatureMax(), 99.0);
    }

    void trueMinMax_pressureInMbar()
    {
        WeatherDataModel model;

        BarReading br;
        br.pressureSeaLevel = 29.80;
        model.applyBarUpdate(br);
        br.pressureSeaLevel = 30.20;
        model.applyBarUpdate(br);
        br.pressureSeaLevel = 30.00;
        model.applyBarUpdate(br);

        // pressureMin/Max are in inHg (raw), DashboardGrid converts to mbar
        QCOMPARE(model.pressureMin(), 29.80);
        QCOMPARE(model.pressureMax(), 30.20);
    }

    void trueMinMax_windSpeedWithDecimation()
    {
        // Specific test for the reported bug: wind speed max marker drifting
        WeatherDataModel model;

        // Simulate 24h of mostly calm wind with one gust
        for (int i = 0; i < 1000; i++)
            model.applyIssUpdate(makeIss(70.0, 50.0, 0, 0, 0, /*windSpeed=*/2.0));

        // One gust to 45 mph
        model.applyIssUpdate(makeIss(70.0, 50.0, 0, 0, 0, /*windSpeed=*/45.0));

        // Back to calm
        for (int i = 0; i < 1000; i++)
            model.applyIssUpdate(makeIss(70.0, 50.0, 0, 0, 0, /*windSpeed=*/3.0));

        // True max must be 45.0, not a strided sample that misses it
        QCOMPARE(model.windSpeedMax(), 45.0);
        QCOMPARE(model.windSpeedMin(), 2.0);
    }

    void trueMinMax_aqiTracked()
    {
        qint64 fakeMs = 0;
        WeatherDataModel model(nullptr, makeClock(&fakeMs));

        PurpleAirReading r;
        r.pm25avg = 10.0;
        r.pm10 = 5.0;
        r.aqi = 42.0;
        model.applyPurpleAirUpdate(r);

        r.aqi = 15.0;
        model.applyPurpleAirUpdate(r);

        r.aqi = 85.0;
        model.applyPurpleAirUpdate(r);

        r.aqi = 50.0;
        model.applyPurpleAirUpdate(r);

        QCOMPARE(model.aqiMin(), 15.0);
        QCOMPARE(model.aqiMax(), 85.0);
    }

    void windRose_existingBehaviorPreserved()
    {
        // Non-calm samples still update directional bin counts and avgSpeed correctly
        qint64 fakeMs = 0;
        WeatherDataModel model(nullptr, makeClock(&fakeMs));

        UdpReading udp;
        udp.windDirLast = 45;  // NE, bin 2
        udp.windSpeedLast = 8.0;
        udp.windSpeedHi10 = 10.0;
        udp.rainRateLast = 0.0;
        udp.rainfallDaily = 0.0;
        udp.rainSize = 1;
        model.applyUdpUpdate(udp);

        udp.windSpeedLast = 12.0;
        model.applyUdpUpdate(udp);

        QVariantList data = model.windRoseData();
        QVariantMap neBin = data[2].toMap();
        QCOMPARE(neBin[QStringLiteral("count")].toInt(), 2);
        // avgSpeed = (8+12)/2 = 10.0
        QVERIFY(qAbs(neBin[QStringLiteral("avgSpeed")].toDouble() - 10.0) < 0.01);
    }
};

QTEST_MAIN(TstWeatherDataModel)
#include "tst_WeatherDataModel.moc"
