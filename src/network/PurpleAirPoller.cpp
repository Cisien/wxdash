#include "network/PurpleAirPoller.h"
#include "network/JsonParser.h"

PurpleAirPoller::PurpleAirPoller(const QUrl &url, QObject *parent)
    : JsonPoller(url, 30000, parent)
{
}

void PurpleAirPoller::handleResponse(const QByteArray &data)
{
    emit purpleAirReceived(JsonParser::parsePurpleAirJson(data));
}
