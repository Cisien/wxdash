#include "network/NwsPoller.h"
#include "network/JsonParser.h"

#include <QNetworkRequest>

NwsPoller::NwsPoller(const QUrl &url, QObject *parent)
    : JsonPoller(url, 30 * 60 * 1000, parent)
{
}

void NwsPoller::configureRequest(QNetworkRequest &req)
{
    req.setRawHeader("User-Agent", "wxdash/1.0 (github.com/cisien/wxdash)");
}

bool NwsPoller::validateReply(QNetworkReply *reply)
{
    int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    return httpStatus == 200;
}

void NwsPoller::handleResponse(const QByteArray &data)
{
    auto forecast = JsonParser::parseForecast(data);
    if (!forecast.isEmpty())
        emit forecastReceived(forecast);
}
