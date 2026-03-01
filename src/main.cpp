#include "models/WeatherDataModel.h"
#include "models/WeatherReadings.h"
#include "network/HttpPoller.h"
#include "network/UdpReceiver.h"

#include <QCoreApplication>
#include <QDebug>
#include <QThread>
#include <QUrl>
#include <QUrlQuery>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    // Register cross-thread data types so Qt queued connections can copy them.
    qRegisterMetaType<IssReading>();
    qRegisterMetaType<BarReading>();
    qRegisterMetaType<IndoorReading>();
    qRegisterMetaType<UdpReading>();

    // WeatherDataModel lives on the main thread.
    // Phase 2 GUI widgets will bind to its Q_PROPERTY NOTIFY signals from the same thread.
    auto *model = new WeatherDataModel(&app);

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
    auto *udpReceiver = new UdpReceiver(realtimeUrl);

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

    // Thread lifecycle: start workers when thread starts, delete them when thread finishes.
    QObject::connect(networkThread, &QThread::started, httpPoller, &HttpPoller::start);
    QObject::connect(networkThread, &QThread::started, udpReceiver, &UdpReceiver::start);
    QObject::connect(networkThread, &QThread::finished, httpPoller, &QObject::deleteLater);
    QObject::connect(networkThread, &QThread::finished, udpReceiver, &QObject::deleteLater);

    // Diagnostic connections for Phase 1 end-to-end validation.
    // These prove data actually flows from the WeatherLink Live all the way into the model.
    QObject::connect(model, &WeatherDataModel::temperatureChanged, [](double value) {
        qDebug() << "Temperature:" << value;
    });
    QObject::connect(model, &WeatherDataModel::sourceStaleChanged, [](bool stale) {
        qDebug() << "Source stale:" << stale;
    });

    // Start the network thread — fires QThread::started, which triggers start() on both workers.
    networkThread->start();

    // Clean shutdown: stop the network thread gracefully when the application quits.
    QObject::connect(&app, &QCoreApplication::aboutToQuit, [networkThread]() {
        networkThread->quit();
        networkThread->wait();
    });

    return app.exec();
}
