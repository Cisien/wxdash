import QtQuick
import QtQuick.Layouts

Item {
    id: root

    property var forecastData: []   // QVariantList of QVariantMaps from weatherModel

    function iconPath(code) {
        var map = {
            "skc": "sun", "nskc": "sun", "hot": "sun",
            "few": "sun_cloud", "nfew": "sun_cloud",
            "sct": "partly_cloudy", "nsct": "partly_cloudy",
            "bkn": "mostly_cloudy", "nbkn": "mostly_cloudy",
            "ovc": "cloudy", "novc": "cloudy",
            "ra": "rain", "nra": "rain",
            "minus_ra": "drizzle",
            "shra": "rain_showers", "nshra": "rain_showers",
            "hi_shwrs": "rain_showers", "hi_nshwrs": "rain_showers",
            "rain_showers": "rain_showers", "rain_showers_hi": "rain_showers",
            "sn": "snow", "nsn": "snow", "cold": "snow", "ncold": "snow",
            "blizzard": "blizzard", "nblizzard": "blizzard",
            "ra_sn": "rain_snow", "nra_sn": "rain_snow",
            "rasn": "rain_snow", "nrasn": "rain_snow",
            "fzra": "freezing_rain", "nfzra": "freezing_rain",
            "ra_fzra": "freezing_rain", "nra_fzra": "freezing_rain",
            "fzra_sn": "freezing_rain", "nfzra_sn": "freezing_rain",
            "ip": "sleet", "nip": "sleet",
            "snip": "sleet", "nsnip": "sleet",
            "raip": "sleet", "nraip": "sleet",
            "tsra": "thunderstorm", "ntsra": "thunderstorm",
            "tsra_sct": "thunderstorm", "tsra_hi": "thunderstorm",
            "scttsra": "thunderstorm", "nscttsra": "thunderstorm",
            "hi_tsra": "thunderstorm", "hi_ntsra": "thunderstorm",
            "fc": "thunderstorm", "tor": "thunderstorm",
            "hur_warn": "thunderstorm", "hur_watch": "thunderstorm",
            "ts_warn": "thunderstorm", "ts_watch": "thunderstorm",
            "wind_skc": "windy", "nwind_skc": "windy",
            "wind_few": "windy", "wind_sct": "windy",
            "wind_bkn": "windy", "wind_ovc": "windy",
            "fg": "fog", "nfg": "fog",
            "hz": "fog", "du": "fog", "ndu": "fog",
            "fu": "fog", "nfu": "fog",
            // Full-word NWS icon codes (newer API format)
            "rain": "rain", "rain_likely": "rain",
            "snow": "snow", "fog": "fog",
            "thunderstorm": "thunderstorm",
            "rain_showers_likely": "rain_showers"
        };
        var name = map[code] || "unknown";
        return "qrc:/icons/weather/" + name + ".svg";
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 6
        spacing: 4

        Repeater {
            model: root.forecastData

            delegate: Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                readonly property var day: modelData

                Column {
                    anchors.centerIn: parent
                    spacing: 2

                    // Weather icon
                    Image {
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: Math.min(parent.parent.width, parent.parent.height) * 0.4
                        height: width
                        source: root.iconPath(day.iconCode || "")
                        sourceSize.width: 64
                        sourceSize.height: 64
                        fillMode: Image.PreserveAspectFit
                    }

                    // High/Low temp on one line
                    Row {
                        anchors.horizontalCenter: parent.horizontalCenter
                        spacing: 1
                        Text {
                            text: day.high > -998 ? day.high + "\u00B0" : "--\u00B0"
                            color: "#C84040"
                            font.pixelSize: Math.min(root.width, root.height) * 0.09
                            font.bold: true
                        }
                        Text {
                            text: "/"
                            color: "#C8A000"
                            font.pixelSize: Math.min(root.width, root.height) * 0.09
                        }
                        Text {
                            text: day.low > -998 ? day.low + "\u00B0" : "--\u00B0"
                            color: "#5B8DD9"
                            font.pixelSize: Math.min(root.width, root.height) * 0.09
                            font.bold: true
                        }
                    }

                    // Precipitation chance
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: day.precip + "%"
                        color: "#C8A000"
                        font.pixelSize: Math.min(root.width, root.height) * 0.07
                    }
                }
            }
        }
    }
}
