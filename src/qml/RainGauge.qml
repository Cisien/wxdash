import QtQuick
import QtQuick.Shapes

Item {
    id: root

    // Public API
    property real rainRate:     0.0   // in/hr
    property real dailyTotal:   0.0   // in
    property var sparklineData: []    // QVariantList for rain rate history

    // Arc geometry constants — match ArcGauge proportions
    readonly property real arcStartAngle: 125
    readonly property real arcSweepAngle: 290

    // Total stroke width matches ArcGauge
    readonly property real totalStrokeWidth: Math.min(width, height) * 0.08

    // Partition: rain rate outer 60%, daily total inner 40%
    readonly property real rateStroke:  totalStrokeWidth * 0.6
    readonly property real dailyStroke: totalStrokeWidth * 0.4

    // Center radius of each ring
    readonly property real outerRadius: (Math.min(width, height) / 2) - (rateStroke / 2)
    readonly property real innerRadius: outerRadius - (rateStroke / 2) - (dailyStroke / 2)

    // Gauge ranges
    readonly property real rateMax:  1.5  // in/hr
    readonly property real dailyMax: 2.0  // in/day

    // Sweep calculations
    readonly property real rateTargetSweep:  arcSweepAngle * Math.max(0, Math.min(1, rainRate / rateMax))
    readonly property real dailyTargetSweep: arcSweepAngle * Math.max(0, Math.min(1, dailyTotal / dailyMax))

    // Rain rate arc color
    function rateColor(rate) {
        if (rate >= 1.0) return "#C84040"  // red — heavy
        if (rate >= 0.3) return "#C87C2A"  // orange — moderate
        if (rate >= 0.1) return "#C8A000"  // yellow — light
        return "#5CA85C"                   // green — trace/none
    }

    // Min/max from sparkline (cache in local var to avoid repeated C++ getter calls)
    readonly property real sparklineMin: {
        var d = sparklineData
        if (!d || d.length === 0) return rainRate
        var m = d[0]
        for (var i = 1; i < d.length; i++)
            if (d[i] < m) m = d[i]
        return m
    }
    readonly property real sparklineMax: {
        var d = sparklineData
        if (!d || d.length === 0) return rainRate
        var m = d[0]
        for (var i = 1; i < d.length; i++)
            if (d[i] > m) m = d[i]
        return m
    }

    // Sparkline background overlay — gold only
    Canvas {
        id: sparklineCanvas
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        height: parent.height / 3
        z: -1

        onWidthChanged:  requestPaint()
        onHeightChanged: requestPaint()

        property var data: root.sparklineData
        onDataChanged: requestPaint()

        onPaint: {
            var ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)
            if (!data || data.length < 2) return

            var count = data.length
            var stride = Math.max(1, Math.floor(count / width))

            var minV = 0, maxV = root.rateMax
            for (var i = 0; i < count; i++) {
                if (data[i] > maxV) maxV = data[i]
            }
            var range = maxV - minV
            if (range === 0) range = 1

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

    // Shape with tracks and fills for both rings
    Shape {
        anchors.fill: parent

        // 1. Rain rate track (outer)
        ShapePath {
            strokeColor: "#2A2A2A"
            fillColor: "transparent"
            strokeWidth: root.rateStroke
            capStyle: ShapePath.FlatCap
            PathAngleArc {
                centerX: root.width / 2; centerY: root.height / 2
                radiusX: root.outerRadius; radiusY: root.outerRadius
                startAngle: root.arcStartAngle; sweepAngle: root.arcSweepAngle
            }
        }

        // 2. Rain rate fill (outer, color-coded)
        ShapePath {
            id: rateFillPath
            strokeColor: root.rateColor(root.rainRate)
            fillColor: "transparent"
            strokeWidth: root.rateStroke
            capStyle: ShapePath.FlatCap
            property real animatedSweep: 0
            Behavior on animatedSweep {
                SmoothedAnimation { velocity: 200 }
            }
            Component.onCompleted: animatedSweep = root.rateTargetSweep
            PathAngleArc {
                centerX: root.width / 2; centerY: root.height / 2
                radiusX: root.outerRadius; radiusY: root.outerRadius
                startAngle: root.arcStartAngle
                sweepAngle: rateFillPath.animatedSweep
            }
        }

        // 3. Daily total track (inner)
        ShapePath {
            strokeColor: "#2A2A2A"
            fillColor: "transparent"
            strokeWidth: root.dailyStroke
            capStyle: ShapePath.FlatCap
            PathAngleArc {
                centerX: root.width / 2; centerY: root.height / 2
                radiusX: root.innerRadius; radiusY: root.innerRadius
                startAngle: root.arcStartAngle; sweepAngle: root.arcSweepAngle
            }
        }

        // 4. Daily total fill (inner, neutral yellow)
        ShapePath {
            id: dailyFillPath
            strokeColor: "#C8A000"
            fillColor: "transparent"
            strokeWidth: root.dailyStroke
            capStyle: ShapePath.FlatCap
            property real animatedSweep: 0
            Behavior on animatedSweep {
                SmoothedAnimation { velocity: 200 }
            }
            Component.onCompleted: animatedSweep = root.dailyTargetSweep
            PathAngleArc {
                centerX: root.width / 2; centerY: root.height / 2
                radiusX: root.innerRadius; radiusY: root.innerRadius
                startAngle: root.arcStartAngle
                sweepAngle: dailyFillPath.animatedSweep
            }
        }
    }

    // Bindings to drive animated sweeps
    Binding { target: rateFillPath;  property: "animatedSweep"; value: root.rateTargetSweep  }
    Binding { target: dailyFillPath; property: "animatedSweep"; value: root.dailyTargetSweep }

    // Min/max tick marks on outer arc — drawn on top
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

            var cx = width / 2
            var cy = height / 2
            var r = root.outerRadius
            var halfStroke = root.rateStroke / 2

            function drawTick(val, color) {
                var ratio = Math.max(0, Math.min(1, val / root.rateMax))
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

    // Inner text layout
    Column {
        anchors.centerIn: parent
        width: root.innerRadius * 1.4
        spacing: 0

        Text {
            width: parent.width
            text: "Rain Rate"
            color: "#C8A000"
            font.pixelSize: Math.min(root.width, root.height) * 0.09
            horizontalAlignment: Text.AlignHCenter
            fontSizeMode: Text.HorizontalFit
            minimumPixelSize: 8
        }

        Text {
            width: parent.width
            text: root.rainRate.toFixed(2)
            color: "#C8A000"
            font.pixelSize: Math.min(root.width, root.height) * 0.18
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
            fontSizeMode: Text.HorizontalFit
            minimumPixelSize: 10
        }

        Text {
            width: parent.width
            text: "in/hr"
            color: "#8A7A00"
            font.pixelSize: Math.min(root.width, root.height) * 0.075
            horizontalAlignment: Text.AlignHCenter
        }

        Text {
            width: parent.width
            text: "Daily: " + root.dailyTotal.toFixed(2) + " in"
            color: "#C8A000"
            font.pixelSize: Math.min(root.width, root.height) * 0.065
            horizontalAlignment: Text.AlignHCenter
            fontSizeMode: Text.HorizontalFit
            minimumPixelSize: 8
        }
    }
}
