import QtQuick

Item {
    id: root

    // Wind direction in degrees (0=North, 90=East, 180=South, 270=West)
    property int windDir: 0
    // Wind rose histogram: list of 16 objects {count, avgSpeed}, one per compass bin
    property var windRoseData: []
    // Maximum sample count across all bins (for normalization)
    property int windRoseMaxCount: 0

    function directionLabel(deg) {
        var dirs = ["N","NNE","NE","ENE","E","ESE","SE","SSE",
                    "S","SSW","SW","WSW","W","WNW","NW","NNW"]
        return dirs[Math.round(deg / 22.5) % 16]
    }

    function windSpeedColor(mph) {
        if (mph < 5)   return "#5CA85C"
        if (mph < 15)  return "#C8A000"
        if (mph < 30)  return "#C87C2A"
        return "#C84040"
    }

    // Title label
    Text {
        id: titleText
        text: "Wind Dir"
        color: "#C8A000"
        font.pixelSize: parent.height * 0.07
        font.bold: false
        anchors.top: parent.top
        anchors.topMargin: parent.height * 0.02
        anchors.horizontalCenter: parent.horizontalCenter
    }

    Item {
        id: roseArea
        anchors.top: titleText.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: dirLabel.top
        anchors.margins: 4

        // Static background: concentric circles, crosshairs, compass labels
        Canvas {
            id: bgCanvas
            anchors.fill: parent

            onWidthChanged: requestPaint()
            onHeightChanged: requestPaint()

            onPaint: {
                var ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)

                var cx = width / 2
                var cy = height / 2
                var outerR = Math.min(width, height) * 0.5 * 0.82

                var gold = "#C8A000"
                var dimGold = "#5A4A00"
                var ringColor = "#3A3A3A"
                var darkBg = "#222222"

                // Dark circle background
                ctx.beginPath()
                ctx.arc(cx, cy, outerR, 0, 2 * Math.PI)
                ctx.fillStyle = darkBg
                ctx.fill()

                // Three concentric reference circles
                var rings = [0.33, 0.66, 1.0]
                for (var r = 0; r < rings.length; r++) {
                    ctx.beginPath()
                    ctx.arc(cx, cy, outerR * rings[r], 0, 2 * Math.PI)
                    ctx.strokeStyle = (r === 2) ? "#4A4A4A" : ringColor
                    ctx.lineWidth = (r === 2) ? 1.5 : 0.7
                    ctx.stroke()
                }

                // Crosshair lines
                ctx.beginPath()
                ctx.moveTo(cx, cy - outerR)
                ctx.lineTo(cx, cy + outerR)
                ctx.moveTo(cx - outerR, cy)
                ctx.lineTo(cx + outerR, cy)
                ctx.strokeStyle = ringColor
                ctx.lineWidth = 0.7
                ctx.stroke()

                // 16-point compass labels outside the circle
                ctx.textAlign = "center"
                ctx.textBaseline = "middle"

                var points = [
                    { a: 0,     t: "N",   s: 0.15, b: true  },
                    { a: 22.5,  t: "NNE", s: 0.065, b: false },
                    { a: 45,    t: "NE",  s: 0.10, b: false },
                    { a: 67.5,  t: "ENE", s: 0.065, b: false },
                    { a: 90,    t: "E",   s: 0.15, b: true  },
                    { a: 112.5, t: "ESE", s: 0.065, b: false },
                    { a: 135,   t: "SE",  s: 0.10, b: false },
                    { a: 157.5, t: "SSE", s: 0.065, b: false },
                    { a: 180,   t: "S",   s: 0.15, b: true  },
                    { a: 202.5, t: "SSW", s: 0.065, b: false },
                    { a: 225,   t: "SW",  s: 0.10, b: false },
                    { a: 247.5, t: "WSW", s: 0.065, b: false },
                    { a: 270,   t: "W",   s: 0.15, b: true  },
                    { a: 292.5, t: "WNW", s: 0.065, b: false },
                    { a: 315,   t: "NW",  s: 0.10, b: false },
                    { a: 337.5, t: "NNW", s: 0.065, b: false }
                ]

                for (var i = 0; i < points.length; i++) {
                    var p = points[i]
                    var fontSize = Math.max(8, Math.round(outerR * p.s))
                    ctx.font = (p.b ? "bold " : "") + fontSize + "px sans-serif"
                    ctx.fillStyle = p.b ? gold : dimGold
                    var labelR = outerR * (p.b ? 1.14 : (p.t.length === 2 ? 1.14 : 1.16))
                    var lRad = p.a * Math.PI / 180
                    ctx.fillText(p.t,
                                 cx + labelR * Math.sin(lRad),
                                 cy - labelR * Math.cos(lRad))
                }
            }
        }

        // Wind rose radial bars — drawn from accumulated histogram data
        Canvas {
            id: barsCanvas
            anchors.fill: parent

            property var roseData: root.windRoseData
            property int maxCount: root.windRoseMaxCount

            onRoseDataChanged: requestPaint()
            onMaxCountChanged: requestPaint()
            onWidthChanged: requestPaint()
            onHeightChanged: requestPaint()

            onPaint: {
                var ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)

                var data = roseData
                var maxC = maxCount
                if (!data || data.length < 16 || maxC <= 0) return

                var cx = width / 2
                var cy = height / 2
                var outerR = Math.min(width, height) * 0.5 * 0.82

                var minBarR = outerR * 0.08  // minimum bar length for any non-zero bin
                var maxBarR = outerR * 0.88  // maximum bar reaches near the outer ring
                var wedgeHalfAngle = 9.5     // slightly less than 11.25° for gap between bars
                var deg2rad = Math.PI / 180

                for (var i = 0; i < 16; i++) {
                    var bin = data[i]
                    if (!bin || bin.count <= 0) continue

                    var centerAngle = i * 22.5
                    var barLength = minBarR + (bin.count / maxC) * (maxBarR - minBarR)
                    var color = root.windSpeedColor(bin.avgSpeed)

                    // Draw wedge sector from center outward
                    var startRad = (centerAngle - wedgeHalfAngle - 90) * deg2rad
                    var endRad = (centerAngle + wedgeHalfAngle - 90) * deg2rad

                    ctx.beginPath()
                    ctx.moveTo(cx, cy)
                    ctx.arc(cx, cy, barLength, startRad, endRad)
                    ctx.closePath()
                    ctx.fillStyle = color
                    ctx.globalAlpha = 0.75
                    ctx.fill()

                    // Subtle edge highlight
                    ctx.strokeStyle = Qt.lighter(color, 1.3)
                    ctx.lineWidth = 0.8
                    ctx.globalAlpha = 0.4
                    ctx.stroke()
                }

                ctx.globalAlpha = 1.0
            }
        }

        // Direction pointer — thin needle for current wind direction
        Item {
            id: pointerWrapper
            anchors.fill: parent
            rotation: root.windDir

            Behavior on rotation {
                RotationAnimation {
                    duration: 400
                    direction: RotationAnimation.Shortest
                }
            }

            Canvas {
                id: pointerCanvas
                anchors.fill: parent

                onWidthChanged: requestPaint()
                onHeightChanged: requestPaint()

                onPaint: {
                    var ctx = getContext("2d")
                    ctx.clearRect(0, 0, width, height)

                    var cx = width / 2
                    var cy = height / 2
                    var outerR = Math.min(width, height) * 0.5 * 0.82

                    // Needle line from center to near edge
                    var needleLen = outerR * 0.92
                    var needleHalf = outerR * 0.025

                    // Thin tapered needle pointing up (rotation handled by parent)
                    ctx.beginPath()
                    ctx.moveTo(cx, cy - needleLen)          // tip
                    ctx.lineTo(cx - needleHalf, cy - outerR * 0.15) // left shoulder
                    ctx.lineTo(cx - needleHalf * 0.5, cy)   // left base
                    ctx.lineTo(cx + needleHalf * 0.5, cy)   // right base
                    ctx.lineTo(cx + needleHalf, cy - outerR * 0.15) // right shoulder
                    ctx.closePath()
                    ctx.fillStyle = "#FFFFFF"
                    ctx.globalAlpha = 0.9
                    ctx.fill()

                    // Outline for contrast against bars
                    ctx.strokeStyle = "#000000"
                    ctx.lineWidth = 1
                    ctx.globalAlpha = 0.5
                    ctx.stroke()

                    ctx.globalAlpha = 1.0

                    // Center hub
                    ctx.beginPath()
                    ctx.arc(cx, cy, outerR * 0.045, 0, 2 * Math.PI)
                    ctx.fillStyle = "#333333"
                    ctx.fill()
                    ctx.strokeStyle = "#FFFFFF"
                    ctx.lineWidth = 1.5
                    ctx.globalAlpha = 0.9
                    ctx.stroke()
                    ctx.globalAlpha = 1.0
                }
            }
        }
    }

    // Direction label with degrees
    Text {
        id: dirLabel
        text: directionLabel(windDir) + "  " + windDir + "\u00B0"
        color: "#C8A000"
        font.pixelSize: parent.height * 0.08
        font.bold: true
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
    }
}
