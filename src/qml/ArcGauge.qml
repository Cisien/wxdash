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

    // Min/max computed from sparkline data.
    // IMPORTANT: cache sparklineData in a local var — direct property access in a
    // loop triggers the C++ getter (sparklineToList) on every iteration in AOT mode.
    readonly property real sparklineMin: {
        var d = sparklineData
        if (!d || d.length === 0) return value
        var m = d[0]
        for (var i = 1; i < d.length; i++)
            if (d[i] < m) m = d[i]
        return m
    }
    readonly property real sparklineMax: {
        var d = sparklineData
        if (!d || d.length === 0) return value
        var m = d[0]
        for (var i = 1; i < d.length; i++)
            if (d[i] > m) m = d[i]
        return m
    }

    // Sparkline background overlay (renders behind gauge arc and text)
    Canvas {
        id: sparklineCanvas
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        height: parent.height / 3      // lower third of cell
        z: -1                           // behind gauge arcs and text

        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()

        // Repaint when data changes
        property var data: root.sparklineData
        onDataChanged: requestPaint()

        onPaint: {
            var ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)
            if (!data || data.length < 2) return

            var count = data.length

            // Decimation: stride through data to fit pixel width
            var stride = Math.max(1, Math.floor(count / width))

            // Use gauge min/max as baseline, expand if data exceeds
            var minV = root.minValue, maxV = root.maxValue
            for (var i = 0; i < count; i++) {
                if (data[i] < minV) minV = data[i]
                if (data[i] > maxV) maxV = data[i]
            }
            var range = maxV - minV
            if (range === 0) range = 1  // flat line — avoid div by zero

            ctx.beginPath()
            ctx.strokeStyle = "#5A4500"   // dim gold
            ctx.lineWidth = 1
            ctx.lineJoin = "round"

            var first = true
            for (var j = 0; j < count; j += stride) {
                var x = (j / (count - 1)) * width
                var y = height - ((data[j] - minV) / range) * height * 0.9
                if (first) { ctx.moveTo(x, y); first = false }
                else ctx.lineTo(x, y)
            }
            ctx.stroke()
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

    // Drive animatedSweep from targetSweep via a binding on the root
    Binding {
        target: fillPath
        property: "animatedSweep"
        value: root.targetSweep
    }

    // Min/max tick mark overlay — drawn on top of arcs
    Canvas {
        id: minMaxCanvas
        anchors.fill: parent
        z: 10

        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()

        property real sMin: root.sparklineMin
        property real sMax: root.sparklineMax
        onSMinChanged: requestPaint()
        onSMaxChanged: requestPaint()

        onPaint: {
            var ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)
            if (!root.sparklineData || root.sparklineData.length < 2) return
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

            drawTick(sMin, "#5B8DD9")  // blue for min
            drawTick(sMax, "#C84040")  // red for max
        }
    }

    // Inner radius boundary for text sizing
    readonly property real innerRadius: (Math.min(width, height) / 2) - strokeWidth

    // Vertical layout: all text stacked in a column, centered in the gauge
    Column {
        anchors.centerIn: parent
        width: root.innerRadius * 1.4
        spacing: 0

        // Label text (gauge title)
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

        // Value text (number only)
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

        // Unit text (small, dimmer)
        Text {
            width: parent.width
            visible: root.unit !== ""
            text: root.unit
            color: "#8A7A00"
            font.pixelSize: Math.min(root.width, root.height) * 0.075
            horizontalAlignment: Text.AlignHCenter
        }

        // Secondary text (optional)
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
