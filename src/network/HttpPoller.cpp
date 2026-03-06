#include "network/HttpPoller.h"
#include "network/JsonParser.h"

HttpPoller::HttpPoller(const QUrl &url, QObject *parent)
    : JsonPoller(url, 10000, parent)
{
}

void HttpPoller::handleResponse(const QByteArray &data)
{
    const auto parsed = JsonParser::parseCurrentConditions(data);
    if (parsed.iss)
        emit issReceived(*parsed.iss);
    if (parsed.bar)
        emit barReceived(*parsed.bar);
    if (parsed.indoor)
        emit indoorReceived(*parsed.indoor);
}
