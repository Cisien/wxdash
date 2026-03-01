#pragma once

#include <cstdint>

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

// Required for cross-thread queued connections
#include <QMetaType>
Q_DECLARE_METATYPE(IssReading)
Q_DECLARE_METATYPE(BarReading)
Q_DECLARE_METATYPE(IndoorReading)
Q_DECLARE_METATYPE(UdpReading)
