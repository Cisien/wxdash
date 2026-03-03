import QtQuick
import QtQuick.Layouts

Item {
    id: dashboard
    anchors.fill: parent

    // --- Threshold color functions ---

    function temperatureColor(tempF) {
        if (tempF < 32)  return "#5B8DD9"   // subdued blue
        if (tempF < 50)  return "#7BB8E8"   // subdued light blue
        if (tempF < 70)  return "#5CA85C"   // subdued green
        if (tempF < 85)  return "#C8A000"   // subdued yellow
        if (tempF < 100) return "#C87C2A"   // subdued orange
        return "#C84040"                    // subdued red
    }

    function humidityColor(hum) {
        if (hum < 25)  return "#C84040"     // subdued red (too dry)
        if (hum < 30)  return "#C87C2A"     // subdued orange
        if (hum < 60)  return "#5CA85C"     // subdued green (comfortable)
        if (hum < 70)  return "#C8A000"     // subdued yellow
        return "#C84040"                    // subdued red (too humid)
    }

    function windSpeedColor(mph) {
        if (mph < 5)   return "#5CA85C"     // subdued green
        if (mph < 15)  return "#C8A000"     // subdued yellow
        if (mph < 30)  return "#C87C2A"     // subdued orange
        return "#C84040"                    // subdued red
    }

    function uvColor(uv) {
        if (uv <= 2)  return "#5CA85C"      // subdued green
        if (uv <= 5)  return "#C8A000"      // subdued yellow
        if (uv <= 7)  return "#C87C2A"      // subdued orange
        if (uv <= 10) return "#C84040"      // subdued red
        return "#8B5CA8"                    // subdued violet
    }

    function aqiColor(aqi) {
        if (aqi <= 50)  return "#5CA85C"    // Green (Good)
        if (aqi <= 100) return "#C8A000"    // Yellow (Moderate)
        if (aqi <= 150) return "#C87C2A"    // Orange (USG)
        if (aqi <= 200) return "#C84040"    // Red (Unhealthy)
        if (aqi <= 300) return "#8B5CA8"    // Purple (Very Unhealthy)
        return "#7B2828"                     // Maroon (Hazardous)
    }

    function pressureTrendArrow(trend) {
        if (trend === 1)  return "\u2191"   // rising
        if (trend === -1) return "\u2193"   // falling
        return "\u2192"                      // steady
    }

    // --- Feels-like computed properties ---

    property real feelsLikeValue: {
        if (weatherModel.temperature >= 80 && weatherModel.humidity >= 40)
            return weatherModel.heatIndex
        if (weatherModel.temperature <= 50 && weatherModel.windSpeed >= 3)
            return weatherModel.windChill
        return weatherModel.temperature
    }

    property string feelsLikeLabel: {
        if (weatherModel.temperature >= 80 && weatherModel.humidity >= 40)
            return "Heat Index"
        if (weatherModel.temperature <= 50 && weatherModel.windSpeed >= 3)
            return "Wind Chill"
        return "Feels Like"
    }

    // --- Pressure in millibars ---

    property real pressureMbar: weatherModel.pressure * 33.8639

    // --- 3x4 Grid layout ---

    GridLayout {
        anchors.fill: parent
        anchors.margins: 8
        columns: 4
        rows: 3
        uniformCellWidths: true
        uniformCellHeights: true
        columnSpacing: 6
        rowSpacing: 6

        // Row 1

        // Cell 1: Temperature (GAUG-01)
        ArcGauge {
            value: weatherModel.temperature
            minValue: -20
            maxValue: 120
            label: "Temperature"
            unit: "\u00B0F"
            arcColor: temperatureColor(weatherModel.temperature)
            sparklineData: weatherModel.temperatureHistory
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        // Cell 2: Feels-Like (GAUG-09)
        ArcGauge {
            value: feelsLikeValue
            minValue: -20
            maxValue: 120
            label: feelsLikeLabel
            unit: "\u00B0F"
            sparklineData: weatherModel.feelsLikeHistory
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        // Cell 3: Humidity (GAUG-02)
        ArcGauge {
            value: weatherModel.humidity
            minValue: 0
            maxValue: 100
            label: "Humidity"
            unit: "%"
            decimals: 0
            arcColor: humidityColor(weatherModel.humidity)
            sparklineData: weatherModel.humidityHistory
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        // Cell 4: Dew Point (GAUG-10)
        ArcGauge {
            value: weatherModel.dewPoint
            minValue: -20
            maxValue: 80
            label: "Dew Point"
            unit: "\u00B0F"
            sparklineData: weatherModel.dewPointHistory
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        // Row 2

        // Cell 5: Wind Speed (GAUG-04)
        ArcGauge {
            value: weatherModel.windSpeed
            minValue: 0
            maxValue: 60
            label: "Wind"
            unit: "mph"
            arcColor: windSpeedColor(weatherModel.windSpeed)
            secondaryLabel: "Gust"
            secondaryText: weatherModel.windGust.toFixed(1) + " mph"
            sparklineData: weatherModel.windSpeedHistory
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        // Cell 6: Compass Rose (GAUG-05)
        CompassRose {
            windDir: weatherModel.windDir
            windSpeed: weatherModel.windSpeed
            windRoseData: weatherModel.windRoseData
            windRoseMaxCount: weatherModel.windRoseMaxCount
            windRoseDirectionalFraction: weatherModel.windRoseDirectionalFraction
            windSpeedColorFn: function(mph) { return dashboard.windSpeedColor(mph) }
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        // Cell 7: Rain Rate (GAUG-06)
        ArcGauge {
            value: weatherModel.rainRate
            minValue: 0
            maxValue: 4
            label: "Rain Rate"
            unit: "in/hr"
            decimals: 2
            secondaryLabel: "Daily"
            secondaryText: weatherModel.rainfallDaily.toFixed(2) + " in"
            sparklineData: weatherModel.rainRateHistory
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        // Cell 8: Pressure (GAUG-03)
        ArcGauge {
            value: pressureMbar
            minValue: 980
            maxValue: 1050
            label: "Pressure"
            unit: "mbar"
            decimals: 1
            secondaryText: pressureTrendArrow(weatherModel.pressureTrend)
            sparklineData: weatherModel.pressureHistory
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        // Row 3

        // Cell 9: UV Index (GAUG-07)
        ArcGauge {
            value: weatherModel.uvIndex
            minValue: 0
            maxValue: 15
            label: "UV Index"
            unit: ""
            decimals: 1
            arcColor: uvColor(weatherModel.uvIndex)
            sparklineData: weatherModel.uvIndexHistory
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        // Cell 10: Solar Radiation (GAUG-08)
        ArcGauge {
            value: weatherModel.solarRad
            minValue: 0
            maxValue: 1200
            label: "Solar Rad"
            unit: "W/m\u00B2"
            decimals: 0
            sparklineData: weatherModel.solarRadHistory
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        // Cell 11: AQI Multi-Ring Gauge (GAUG-11, GAUG-12)
        AqiGauge {
            aqiValue: weatherModel.aqi
            pm25Value: weatherModel.pm25
            pm10Value: weatherModel.pm10
            aqiColor: dashboard.aqiColor(weatherModel.aqi)
            sparklineData: weatherModel.aqiHistory
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        // Cell 12: 3-Day Forecast
        ForecastPanel {
            forecastData: weatherModel.forecastData
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }
}
