import QtQuick

Item {
    id: root

    // Wind direction in degrees (0=North, 90=East, 180=South, 270=West)
    property int windDir: 0
    // Current wind speed in mph (used for calm detection)
    property real windSpeed: 0
    // Wind rose histogram: list of 16 objects {count, avgSpeed, recentAvgSpeed}, one per compass bin
    property var windRoseData: []
    // Maximum sample count across all bins (for normalization)
    property int windRoseMaxCount: 0
    // Fraction of samples that are directional (non-calm) — bars scale by this
    property real windRoseDirectionalFraction: 1.0
    // Wind speed color function — injected from parent dashboard (single source of truth)
    property var windSpeedColorFn: function(mph) { return "#5CA85C" }

    Item {
        id: roseArea
        anchors.fill: parent
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
                var outerR = Math.min(width, height) * 0.5 * 0.95

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

                // Tick marks at cardinal and intercardinal points
                for (var t = 0; t < 16; t++) {
                    var tickAngle = t * 22.5
                    var tickRad = tickAngle * Math.PI / 180
                    var isCardinal = (t % 4 === 0)
                    var innerTick = isCardinal ? outerR * 0.92 : outerR * 0.95
                    ctx.beginPath()
                    ctx.moveTo(cx + innerTick * Math.sin(tickRad),
                               cy - innerTick * Math.cos(tickRad))
                    ctx.lineTo(cx + outerR * Math.sin(tickRad),
                               cy - outerR * Math.cos(tickRad))
                    ctx.strokeStyle = isCardinal ? "#4A4A4A" : ringColor
                    ctx.lineWidth = isCardinal ? 1.5 : 0.7
                    ctx.stroke()
                }
            }
        }

        // Wind rose radial bars — drawn from accumulated histogram data
        Canvas {
            id: barsCanvas
            anchors.fill: parent

            property var roseData: root.windRoseData
            property int maxCount: root.windRoseMaxCount
            property real dirFraction: root.windRoseDirectionalFraction

            onRoseDataChanged: requestPaint()
            onMaxCountChanged: requestPaint()
            onDirFractionChanged: requestPaint()
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
                var outerR = Math.min(width, height) * 0.5 * 0.95

                var minBarR = outerR * 0.08  // minimum bar length for any non-zero bin
                var maxBarR = outerR * 0.88  // maximum bar reaches near the outer ring
                var wedgeHalfAngle = 9.5     // slightly less than 11.25° for gap between bars
                var deg2rad = Math.PI / 180

                for (var i = 0; i < 16; i++) {
                    var bin = data[i]
                    if (!bin || bin.count <= 0) continue

                    var centerAngle = i * 22.5
                    var frac = dirFraction > 0 ? dirFraction : 1.0
                    var barLength = frac * (minBarR + (bin.count / maxC) * (maxBarR - minBarR))
                    var color = root.windSpeedColorFn(bin.recentAvgSpeed !== undefined ? bin.recentAvgSpeed : bin.avgSpeed)

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

                // Track windSpeed to repaint when calm state changes
                property real currentWindSpeed: root.windSpeed
                onCurrentWindSpeedChanged: requestPaint()

                onWidthChanged: requestPaint()
                onHeightChanged: requestPaint()

                onPaint: {
                    var ctx = getContext("2d")
                    ctx.clearRect(0, 0, width, height)

                    var cx = width / 2
                    var cy = height / 2
                    var outerR = Math.min(width, height) * 0.5 * 0.95

                    if (root.windSpeed < 0.1) {
                        // Calm: small center dot only
                        ctx.beginPath()
                        ctx.arc(cx, cy, outerR * 0.04, 0, 2 * Math.PI)
                        ctx.fillStyle = "#C8A000"
                        ctx.fill()
                        return
                    }

                    // Non-calm: draw directional needle

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
                    ctx.fillStyle = gold
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
                    ctx.strokeStyle = gold
                    ctx.lineWidth = 1.5
                    ctx.globalAlpha = 0.9
                    ctx.stroke()
                    ctx.globalAlpha = 1.0
                }
            }
        }
    }

}
