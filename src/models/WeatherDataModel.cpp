#include "models/WeatherDataModel.h"

#include <QTimer>

// Stub implementation — RED phase. Tests will fail.
WeatherDataModel::WeatherDataModel(QObject *parent, std::function<qint64()> elapsedProvider)
    : QObject(parent)
    , m_elapsedProvider(std::move(elapsedProvider))
{
    m_stalenessTimer = new QTimer(this);
    // Intentionally not connected in stub — tests will fail
}

void WeatherDataModel::applyIssUpdate(const IssReading & /*r*/)
{
    // Stub — does nothing
}

void WeatherDataModel::applyBarUpdate(const BarReading & /*r*/)
{
    // Stub — does nothing
}

void WeatherDataModel::applyIndoorUpdate(const IndoorReading & /*r*/)
{
    // Stub — does nothing
}

void WeatherDataModel::applyUdpUpdate(const UdpReading & /*r*/)
{
    // Stub — does nothing
}

void WeatherDataModel::checkStaleness()
{
    // Stub — does nothing
}

void WeatherDataModel::clearAllValues()
{
    // Stub
}

void WeatherDataModel::markUpdated()
{
    // Stub
}
