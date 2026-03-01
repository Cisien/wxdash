import QtQuick

Item {
    id: root

    // Wind direction in degrees (0=North, 90=East, 180=South, 270=West)
    property int windDir: 0

    // Converts numeric degrees to 16-point cardinal label
    function directionLabel(deg) {
        var dirs = ["N","NNE","NE","ENE","E","ESE","SE","SSE",
                    "S","SSW","SW","WSW","W","WNW","NW","NNW"]
        return dirs[Math.round(deg / 22.5) % 16]
    }

    // Title label at the top (matches ArcGauge label position style)
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

        // Static background: concentric circles, crosshairs, tick marks, labels
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
                var faintGold = "#3A2E00"
                var ringColor = "#3A3A3A"
                var darkBg = "#222222"

                // --- Filled dark circle background ---
                ctx.beginPath()
                ctx.arc(cx, cy, outerR, 0, 2 * Math.PI)
                ctx.fillStyle = darkBg
                ctx.fill()

                // --- Three concentric reference circles ---
                var rings = [0.33, 0.66, 1.0]
                for (var r = 0; r < rings.length; r++) {
                    ctx.beginPath()
                    ctx.arc(cx, cy, outerR * rings[r], 0, 2 * Math.PI)
                    ctx.strokeStyle = (r === 2) ? "#4A4A4A" : ringColor
                    ctx.lineWidth = (r === 2) ? 1.5 : 0.7
                    ctx.stroke()
                }

                // --- Crosshair lines (N-S and E-W axes) ---
                ctx.beginPath()
                ctx.moveTo(cx, cy - outerR)
                ctx.lineTo(cx, cy + outerR)
                ctx.moveTo(cx - outerR, cy)
                ctx.lineTo(cx + outerR, cy)
                ctx.strokeStyle = ringColor
                ctx.lineWidth = 0.7
                ctx.stroke()

                // --- Tick marks at all 16 compass points ---
                for (var t = 0; t < 16; t++) {
                    var tickAngle = t * 22.5
                    var tickRad = tickAngle * Math.PI / 180
                    var isCardinal = (t % 4 === 0)
                    var isIntercardinal = (t % 2 === 0 && !isCardinal)
                    var innerR = isCardinal ? outerR * 0.88
                               : isIntercardinal ? outerR * 0.91
                               : outerR * 0.94
                    var lw = isCardinal ? 2.0 : isIntercardinal ? 1.2 : 0.8
                    ctx.beginPath()
                    ctx.moveTo(cx + innerR * Math.sin(tickRad),
                               cy - innerR * Math.cos(tickRad))
                    ctx.lineTo(cx + outerR * Math.sin(tickRad),
                               cy - outerR * Math.cos(tickRad))
                    ctx.strokeStyle = isCardinal ? gold : dimGold
                    ctx.lineWidth = lw
                    ctx.stroke()
                }

                // --- 16-point compass labels outside the circle ---
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

        // Rotating pointer — only this element rotates
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

                    var gold = "#C8A000"
                    var dimGold = "#5A4A00"

                    // --- Direction pointer (drawn at 0°, parent rotation handles wind dir) ---
                    // Tapered arrow pointing north from center to near outer ring

                    var tipY = cy - outerR * 0.85
                    var baseY = cy - outerR * 0.10
                    var shoulderY = cy - outerR * 0.55
                    var halfBase = outerR * 0.09
                    var halfShoulder = outerR * 0.04

                    // Main pointer body — tapered wedge
                    ctx.beginPath()
                    ctx.moveTo(cx, tipY)                        // tip
                    ctx.lineTo(cx - halfShoulder, shoulderY)    // left shoulder
                    ctx.lineTo(cx - halfBase, baseY)            // left base
                    ctx.lineTo(cx + halfBase, baseY)            // right base
                    ctx.lineTo(cx + halfShoulder, shoulderY)    // right shoulder
                    ctx.closePath()
                    ctx.fillStyle = gold
                    ctx.globalAlpha = 0.9
                    ctx.fill()

                    // Pointer outline for definition
                    ctx.strokeStyle = "#E8C800"
                    ctx.lineWidth = 1
                    ctx.globalAlpha = 0.5
                    ctx.stroke()

                    // --- Tail stub (opposite side, subdued) ---
                    var tailTipY = cy + outerR * 0.40
                    var tailBaseY = cy + outerR * 0.10
                    var tailHalf = outerR * 0.06

                    ctx.beginPath()
                    ctx.moveTo(cx, tailTipY)
                    ctx.lineTo(cx - tailHalf, tailBaseY)
                    ctx.lineTo(cx + tailHalf, tailBaseY)
                    ctx.closePath()
                    ctx.fillStyle = dimGold
                    ctx.globalAlpha = 0.6
                    ctx.fill()

                    ctx.globalAlpha = 1.0

                    // --- Center hub ---
                    ctx.beginPath()
                    ctx.arc(cx, cy, outerR * 0.06, 0, 2 * Math.PI)
                    ctx.fillStyle = "#333333"
                    ctx.fill()
                    ctx.strokeStyle = gold
                    ctx.lineWidth = 1.5
                    ctx.stroke()
                }
            }
        }
    }

    // Direction text label below the compass rose
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
