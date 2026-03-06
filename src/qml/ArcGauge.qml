import QtQuick
import QtQuick.Shapes

Item {
    id: root

    // Public API
    property real value: 0.0
    property real minValue: 0.0
    property real maxValue: 100.0
    property string label: ""
    property string unit: ""
    property int decimals: 1
    property color arcColor: "#B8860B"
    property string secondaryText: ""
    property string secondaryLabel: ""
    property var sparklineData: []      // QVariantList from model

    // Arc geometry constants
    readonly property real arcStartAngle: 125
    readonly property real arcSweepAngle: 290
    readonly property real strokeWidth: Math.min(width, height) * 0.08

    // Computed fill sweep based on value position in range
    readonly property real targetSweep: {
        if (maxValue === minValue) return 0
        var ratio = (value - minValue) / (maxValue - minValue)
        return arcSweepAngle * Math.max(0, Math.min(1, ratio))
    }

    // Combined sparkline + min/max Canvas — single data read, painted behind arcs
    Canvas {
        id: sparklineCanvas
        anchors.fill: parent
        z: -1

        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()

        property var data: root.sparklineData
        onDataChanged: requestPaint()

        onPaint: {
            var ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)
            var d = data
            if (!d || d.length < 2) return

            var count = d.length

            // --- Sparkline in lower third ---
            var slHeight = height / 3
            var slTop = height - slHeight
            var stride = Math.max(1, Math.floor(count / width))

            var dataMin = d[0], dataMax = d[0]
            for (var i = 1; i < count; i++) {
                if (d[i] < dataMin) dataMin = d[i]
                if (d[i] > dataMax) dataMax = d[i]
            }

            // Sparkline Y range: gauge range expanded if data exceeds
            var minV = root.minValue, maxV = root.maxValue
            if (dataMin < minV) minV = dataMin
            if (dataMax > maxV) maxV = dataMax
            var range = maxV - minV
            if (range === 0) range = 1

            ctx.beginPath()
            ctx.strokeStyle = "#5A4500"
            ctx.lineWidth = 1
            ctx.lineJoin = "round"

            var first = true
            for (var j = 0; j < count; j += stride) {
                var x = (j / (count - 1)) * width
                var y = slTop + slHeight - ((d[j] - minV) / range) * slHeight * 0.9
                if (first) { ctx.moveTo(x, y); first = false }
                else ctx.lineTo(x, y)
            }
            ctx.stroke()

            // --- Min/max tick marks on the arc ---
            if (root.maxValue === root.minValue) return

            var cx = width / 2
            var cy = height / 2
            var r = (Math.min(width, height) / 2) - (root.strokeWidth / 2)
            var halfStroke = root.strokeWidth / 2

            function drawTick(val, color) {
                var ratio = Math.max(0, Math.min(1, (val - root.minValue) / (root.maxValue - root.minValue)))
                var angleDeg = root.arcStartAngle + root.arcSweepAngle * ratio
                var angleRad = angleDeg * Math.PI / 180
                var cosA = Math.cos(angleRad)
                var sinA = Math.sin(angleRad)
                ctx.beginPath()
                ctx.strokeStyle = color
                ctx.lineWidth = 2
                ctx.moveTo(cx + (r - halfStroke - 2) * cosA, cy + (r - halfStroke - 2) * sinA)
                ctx.lineTo(cx + (r + halfStroke + 2) * cosA, cy + (r + halfStroke + 2) * sinA)
                ctx.stroke()
            }

            drawTick(dataMin, "#5B8DD9")
            drawTick(dataMax, "#C84040")
        }
    }

    // Track arc (full 290-degree background)
    Shape {
        anchors.fill: parent

        ShapePath {
            strokeColor: "#2A2A2A"
            fillColor: "transparent"
            strokeWidth: root.strokeWidth
            capStyle: ShapePath.FlatCap

            PathAngleArc {
                centerX: root.width / 2
                centerY: root.height / 2
                radiusX: (Math.min(root.width, root.height) / 2) - (root.strokeWidth / 2)
                radiusY: radiusX
                startAngle: root.arcStartAngle
                sweepAngle: root.arcSweepAngle
            }
        }
    }

    // Fill arc (animated, colored, value-proportional)
    Shape {
        anchors.fill: parent

        ShapePath {
            id: fillPath
            strokeColor: root.arcColor
            fillColor: "transparent"
            strokeWidth: root.strokeWidth
            capStyle: ShapePath.FlatCap

            property real animatedSweep: 0

            Behavior on animatedSweep {
                SmoothedAnimation {
                    velocity: 200
                }
            }

            Component.onCompleted: animatedSweep = root.targetSweep
            onAnimatedSweepChanged: {}

            PathAngleArc {
                centerX: root.width / 2
                centerY: root.height / 2
                radiusX: (Math.min(root.width, root.height) / 2) - (root.strokeWidth / 2)
                radiusY: radiusX
                startAngle: root.arcStartAngle
                sweepAngle: fillPath.animatedSweep
            }
        }
    }

    Binding {
        target: fillPath
        property: "animatedSweep"
        value: root.targetSweep
    }

    // Inner radius boundary for text sizing
    readonly property real innerRadius: (Math.min(width, height) / 2) - strokeWidth

    Column {
        anchors.centerIn: parent
        width: root.innerRadius * 1.4
        spacing: 0

        Text {
            width: parent.width
            text: root.label
            color: "#C8A000"
            font.pixelSize: Math.min(root.width, root.height) * 0.09
            font.bold: false
            horizontalAlignment: Text.AlignHCenter
            fontSizeMode: Text.HorizontalFit
            minimumPixelSize: 8
        }

        Text {
            id: valueText
            width: parent.width
            text: root.value.toFixed(root.decimals)
            color: "#C8A000"
            font.pixelSize: Math.min(root.width, root.height) * 0.18
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
            fontSizeMode: Text.HorizontalFit
            minimumPixelSize: 10
        }

        Text {
            width: parent.width
            visible: root.unit !== ""
            text: root.unit
            color: "#8A7A00"
            font.pixelSize: Math.min(root.width, root.height) * 0.075
            horizontalAlignment: Text.AlignHCenter
        }

        Text {
            width: parent.width
            visible: root.secondaryText !== ""
            text: root.secondaryLabel !== "" ? root.secondaryLabel + ": " + root.secondaryText : root.secondaryText
            color: "#C8A000"
            font.pixelSize: Math.min(root.width, root.height) * 0.065
            horizontalAlignment: Text.AlignHCenter
            fontSizeMode: Text.HorizontalFit
            minimumPixelSize: 8
        }
    }
}
