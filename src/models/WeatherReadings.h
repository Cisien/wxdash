#pragma once

#include <cstdint>
#include <QString>

/**
 * Plain C++ structs for cross-thread data transfer between network worker thread
 * and the GUI thread. No Qt dependency — trivially copyable and safe for
 * Qt::QueuedConnection signal delivery.
 *
 * Register each type with qRegisterMetaType<T>() in main() before first use.
 */

struct IssReading {
    double temperature = 0.0;   // °F
    double humidity = 0.0;      // %RH
    double dewPoint = 0.0;      // °F
    double heatIndex = 0.0;     // °F
    double windChill = 0.0;     // °F
    double windSpeedLast = 0.0; // mph
    int windDirLast = 0;        // degrees (0-360)
    double windSpeedHi10 = 0.0; // mph (gust, last 10 min)
    double rainRateLast = 0.0;  // inches/hour (converted from counts)
    double rainfallDaily = 0.0; // inches (converted from counts)
    double solarRad = 0.0;      // W/m²
    double uvIndex = 0.0;
    int rainSize = 1; // raw rain_size from API (1–4)
};

struct BarReading {
    double pressureSeaLevel = 0.0; // inHg
    int pressureTrend = 0;         // -1=falling, 0=steady, 1=rising
};

struct IndoorReading {
    double tempIn = 0.0;     // °F
    double humIn = 0.0;      // %RH
    double dewPointIn = 0.0; // °F
};

struct UdpReading {
    double windSpeedLast = 0.0; // mph
    int windDirLast = 0;        // degrees (0-360)
    double windSpeedHi10 = 0.0; // mph (gust)
    double rainRateLast = 0.0;  // inches/hour (converted from counts)
    double rainfallDaily = 0.0; // inches (converted from counts)
    int rainSize = 1;           // raw rain_size from API (1–4)
};

struct PurpleAirReading {
    double pm25_a  = 0.0;  // pm2_5_atm Channel A (ug/m3)
    double pm25_b  = 0.0;  // pm2_5_atm_b Channel B (ug/m3)
    double pm25avg = 0.0;  // averaged (A+B)/2
    double pm10    = 0.0;  // pm10_0_atm (ug/m3)
    double aqi     = 0.0;  // calculated from pm25avg via EPA 2024 breakpoints
};

struct ForecastDay {
    int high    = -999;   // degF daytime high; -999 means "no data" (tonight-only edge case)
    int low     = -999;   // degF nighttime low; -999 means "no data"
    int precip  = 0;      // % precipitation chance (0-100), max of day/night
    QString iconCode;     // NWS icon code: "skc", "tsra", "sn", etc.
};

// Required for cross-thread queued connections
#include <QMetaType>
Q_DECLARE_METATYPE(IssReading)
Q_DECLARE_METATYPE(BarReading)
Q_DECLARE_METATYPE(IndoorReading)
Q_DECLARE_METATYPE(UdpReading)
Q_DECLARE_METATYPE(PurpleAirReading)
Q_DECLARE_METATYPE(ForecastDay)
