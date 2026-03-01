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

    // Canvas for the rotating compass rose
    // anchors.bottomMargin leaves room for the direction label text
    Canvas {
        id: roseCanvas
        anchors.top: titleText.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: dirLabel.top
        anchors.margins: 4

        // Rotate the canvas so the matching compass point faces up.
        // When windDir=0 (North), rotation=0 (N at top).
        // When windDir=90 (East), rotation=-90 (E rotates to top).
        rotation: -windDir

        Behavior on rotation {
            RotationAnimation {
                duration: 400
                direction: RotationAnimation.Shortest
            }
        }

        // Repaint only on resize — rotation is handled by scene-graph transform
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()

        onPaint: {
            var ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)

            var cx = width / 2
            var cy = height / 2
            var radius = Math.min(width, height) * 0.5 * 0.92

            var yellow = "#C8A000"
            var darkBg = "#2A2A2A"

            // --- Outer circle fill ---
            ctx.beginPath()
            ctx.arc(cx, cy, radius * 0.95, 0, 2 * Math.PI)
            ctx.fillStyle = darkBg
            ctx.fill()

            // --- Outer circle stroke ---
            ctx.beginPath()
            ctx.arc(cx, cy, radius * 0.95, 0, 2 * Math.PI)
            ctx.strokeStyle = yellow
            ctx.lineWidth = 1.5
            ctx.stroke()

            // Helper: draw a tick mark at angle 'angleDeg' from inner to outer radius
            function drawTick(angleDeg, innerR, outerR, lineWidth) {
                var rad = angleDeg * Math.PI / 180
                ctx.beginPath()
                ctx.moveTo(cx + innerR * Math.sin(rad), cy - innerR * Math.cos(rad))
                ctx.lineTo(cx + outerR * Math.sin(rad), cy - outerR * Math.cos(rad))
                ctx.strokeStyle = yellow
                ctx.lineWidth = lineWidth
                ctx.stroke()
            }

            // --- Cardinal tick marks (N, E, S, W) — thicker ---
            var cardinals = [0, 90, 180, 270]
            for (var i = 0; i < cardinals.length; i++) {
                drawTick(cardinals[i], radius * 0.80, radius * 0.95, 2.5)
            }

            // --- Intercardinal tick marks (NE, SE, SW, NW) ---
            var intercardinals = [45, 135, 225, 315]
            for (var j = 0; j < intercardinals.length; j++) {
                drawTick(intercardinals[j], radius * 0.85, radius * 0.95, 1.5)
            }

            // --- Secondary intercardinal ticks (NNE, ENE, ESE, SSE, SSW, WSW, WNW, NNW) ---
            var secondary = [22.5, 67.5, 112.5, 157.5, 202.5, 247.5, 292.5, 337.5]
            for (var k = 0; k < secondary.length; k++) {
                drawTick(secondary[k], radius * 0.88, radius * 0.95, 1.0)
            }

            // --- Cardinal direction labels (N, E, S, W) ---
            // Each label is drawn at radius * 0.65 in the appropriate direction.
            // We use save/translate/rotate/fillText/restore so each label appears
            // upright relative to the canvas (rotates with the rose — traditional style).
            var labelRadius = radius * 0.65
            var fontSize = Math.round(radius * 0.16)
            ctx.font = "bold " + fontSize + "px sans-serif"
            ctx.fillStyle = yellow
            ctx.textAlign = "center"
            ctx.textBaseline = "middle"

            var cardinalLabels = [
                { angle: 0,   text: "N" },
                { angle: 90,  text: "E" },
                { angle: 180, text: "S" },
                { angle: 270, text: "W" }
            ]

            for (var l = 0; l < cardinalLabels.length; l++) {
                var lRad = cardinalLabels[l].angle * Math.PI / 180
                var lx = cx + labelRadius * Math.sin(lRad)
                var ly = cy - labelRadius * Math.cos(lRad)
                ctx.save()
                ctx.translate(lx, ly)
                ctx.fillText(cardinalLabels[l].text, 0, 0)
                ctx.restore()
            }

            // --- North pointer (elongated triangle pointing toward North/top) ---
            var pointerLen = radius * 0.75
            var pointerBaseHalf = radius * 0.06
            ctx.beginPath()
            ctx.moveTo(cx, cy - pointerLen)           // tip (pointing North/up)
            ctx.lineTo(cx - pointerBaseHalf, cy)       // left base
            ctx.lineTo(cx + pointerBaseHalf, cy)       // right base
            ctx.closePath()
            ctx.fillStyle = yellow
            ctx.fill()

            // Center dot
            ctx.beginPath()
            ctx.arc(cx, cy, radius * 0.04, 0, 2 * Math.PI)
            ctx.fillStyle = darkBg
            ctx.fill()
            ctx.strokeStyle = yellow
            ctx.lineWidth = 1.5
            ctx.stroke()
        }
    }

    // Direction text label below the compass rose (static — does not rotate)
    Text {
        id: dirLabel
        text: directionLabel(windDir)
        color: "#C8A000"
        font.pixelSize: parent.height * 0.08
        font.bold: true
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
    }
}
