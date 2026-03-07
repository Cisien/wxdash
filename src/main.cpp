#include "models/WeatherDataModel.h"
#include "models/WeatherReadings.h"
#include "network/HttpPoller.h"
#include "network/NwsPoller.h"
#include "network/PurpleAirPoller.h"
#include "network/UdpReceiver.h"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDir>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QStandardPaths>
#include <QThread>
#include <QIcon>
#include <QUrl>
#include <QUrlQuery>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setWindowIcon(QIcon(QStringLiteral(":/icons/wxdash.svg")));

    QCommandLineParser parser;
    parser.addHelpOption();
    QCommandLineOption kioskOption("kiosk", "Launch fullscreen frameless");
    QCommandLineOption verboseOption("verbose", "Log all HTTP requests/responses and UDP traffic");
    parser.addOption(kioskOption);
    parser.addOption(verboseOption);
    parser.process(app);
    bool kioskMode = parser.isSet(kioskOption);
    bool verbose = parser.isSet(verboseOption);

    // Register cross-thread data types so Qt queued connections can copy them.
    qRegisterMetaType<IssReading>();
    qRegisterMetaType<BarReading>();
    qRegisterMetaType<IndoorReading>();
    qRegisterMetaType<UdpReading>();
    qRegisterMetaType<PurpleAirReading>();
    qRegisterMetaType<QVector<ForecastDay>>("QVector<ForecastDay>");

    // WeatherDataModel lives on the main thread.
    // QML gauges bind to its Q_PROPERTY NOTIFY signals from the same thread.
    auto *model = new WeatherDataModel(&app);

    // Load persisted sparkline history so graphs show data immediately on startup.
    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);
    const QString sparklinePath = dataDir + QStringLiteral("/sparklines.bin");
    model->loadSparklineData(sparklinePath);

    // HTTP polling endpoint.
    const QUrl httpUrl(QStringLiteral("http://weatherlinklive.local.cisien.com/v1/current_conditions"));

    // UDP realtime broadcast endpoint: duration=86400 seconds per API spec.
    QUrl realtimeBase(QStringLiteral("http://weatherlinklive.local.cisien.com/v1/real_time"));
    QUrlQuery realtimeQuery;
    realtimeQuery.addQueryItem(QStringLiteral("duration"), QStringLiteral("86400"));
    realtimeBase.setQuery(realtimeQuery);
    const QUrl realtimeUrl = realtimeBase;

    // Network thread — HttpPoller and UdpReceiver share it.
    auto *networkThread = new QThread(&app);

    // Worker objects must be created WITHOUT parents before moveToThread.
    auto *httpPoller = new HttpPoller(httpUrl);
    httpPoller->setVerbose(verbose);
    auto *udpReceiver = new UdpReceiver(realtimeUrl);
    udpReceiver->setVerbose(verbose);

    // Move workers to the network thread.
    // QNAM, timers, and sockets are all created inside start() — after moveToThread —
    // so they belong to the correct thread and event loop.
    httpPoller->moveToThread(networkThread);
    udpReceiver->moveToThread(networkThread);

    // Cross-thread signal → slot connections.
    // Qt auto-detects QueuedConnection when sender and receiver live on different threads.
    QObject::connect(httpPoller, &HttpPoller::issReceived, model, &WeatherDataModel::applyIssUpdate);
    QObject::connect(httpPoller, &HttpPoller::barReceived, model, &WeatherDataModel::applyBarUpdate);
    QObject::connect(httpPoller, &HttpPoller::indoorReceived, model, &WeatherDataModel::applyIndoorUpdate);
    QObject::connect(udpReceiver, &UdpReceiver::realtimeReceived, model, &WeatherDataModel::applyUdpUpdate);

    // PurpleAir local sensor polling — shares the network thread with HttpPoller and UdpReceiver
    const QUrl purpleAirUrl(QStringLiteral("http://10.1.255.41/json?live=false"));
    auto *purpleAirPoller = new PurpleAirPoller(purpleAirUrl);
    purpleAirPoller->setVerbose(verbose);
    purpleAirPoller->moveToThread(networkThread);

    QObject::connect(purpleAirPoller, &PurpleAirPoller::purpleAirReceived,
                     model, &WeatherDataModel::applyPurpleAirUpdate);
    QObject::connect(networkThread, &QThread::started,
                     purpleAirPoller, &PurpleAirPoller::start);
    QObject::connect(networkThread, &QThread::finished,
                     purpleAirPoller, &QObject::deleteLater);

    // NWS forecast polling — shares the network thread
    // Grid point SEW/137,72 = Duvall, WA (zip 98019), hardcoded per user decision
    const QUrl nwsUrl(QStringLiteral(
        "https://api.weather.gov/gridpoints/SEW/137,72/forecast"));
    auto *nwsPoller = new NwsPoller(nwsUrl);
    nwsPoller->setVerbose(verbose);
    nwsPoller->moveToThread(networkThread);

    QObject::connect(nwsPoller, &NwsPoller::forecastReceived,
                     model, &WeatherDataModel::applyForecastUpdate);
    QObject::connect(networkThread, &QThread::started,
                     nwsPoller, &NwsPoller::start);
    QObject::connect(networkThread, &QThread::finished,
                     nwsPoller, &QObject::deleteLater);

    // Thread lifecycle: start workers when thread starts, delete them when thread finishes.
    QObject::connect(networkThread, &QThread::started, httpPoller, &HttpPoller::start);
    QObject::connect(networkThread, &QThread::started, udpReceiver, &UdpReceiver::start);
    QObject::connect(networkThread, &QThread::finished, httpPoller, &QObject::deleteLater);
    QObject::connect(networkThread, &QThread::finished, udpReceiver, &QObject::deleteLater);

    // Start the network thread — fires QThread::started, which triggers start() on all workers.
    networkThread->start();

    // Periodic sparkline save every 60 seconds.
    auto *saveTimer = new QTimer(model);
    QObject::connect(saveTimer, &QTimer::timeout, model, [model, sparklinePath]() {
        model->saveSparklineData(sparklinePath);
    });
    saveTimer->start(60000);

    // Clean shutdown: save sparklines and stop the network thread.
    QObject::connect(&app, &QGuiApplication::aboutToQuit, [model, sparklinePath, networkThread]() {
        model->saveSparklineData(sparklinePath);
        networkThread->quit();
        networkThread->wait();
    });

    // QML engine setup — context properties MUST be set before loadFromModule.
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("weatherModel", model);
    engine.rootContext()->setContextProperty("kioskMode", kioskMode);

    // Abort early if QML module fails to load (e.g., missing type in qmldir).
    // Without this check the app enters an empty event loop on EGLFS — no window,
    // no log output, and no response to Ctrl+C.
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, []() { qCritical("QML module failed to load"); qApp->exit(1); },
                     Qt::QueuedConnection);

    engine.loadFromModule("wxdash", "Main");

    if (engine.rootObjects().isEmpty()) {
        qCritical("wxdash: no root QML objects created — check QML_IMPORT_PATH and module install");
        return 1;
    }

    return app.exec();
}
