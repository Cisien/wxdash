import QtQuick
import QtQuick.Shapes

Item {
    id: root

    property real rainRate:     0.0
    property real dailyTotal:   0.0
    property var sparklineData: []

    readonly property real arcStartAngle: 125
    readonly property real arcSweepAngle: 290
    readonly property real totalStrokeWidth: Math.min(width, height) * 0.08
    readonly property real rateStroke:  totalStrokeWidth * 0.6
    readonly property real dailyStroke: totalStrokeWidth * 0.4
    readonly property real outerRadius: (Math.min(width, height) / 2) - (rateStroke / 2)
    readonly property real innerRadius: outerRadius - (rateStroke / 2) - (dailyStroke / 2)
    readonly property real rateMax:  1.5
    readonly property real dailyMax: 2.0
    readonly property real rateTargetSweep:  arcSweepAngle * Math.max(0, Math.min(1, rainRate / rateMax))
    readonly property real dailyTargetSweep: arcSweepAngle * Math.max(0, Math.min(1, dailyTotal / dailyMax))

    function rateColor(rate) {
        if (rate >= 1.0) return "#C84040"
        if (rate >= 0.3) return "#C87C2A"
        if (rate >= 0.1) return "#C8A000"
        return "#5CA85C"
    }

    // Combined sparkline + min/max Canvas
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
            var slHeight = height / 3
            var slTop = height - slHeight
            var stride = Math.max(1, Math.floor(count / width))

            var dataMin = d[0], dataMax = d[0]
            for (var i = 1; i < count; i++) {
                if (d[i] < dataMin) dataMin = d[i]
                if (d[i] > dataMax) dataMax = d[i]
            }

            var minV = 0, maxV = root.rateMax
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

        }
    }

    // Min/max tick marks — separate Canvas above arcs
    Canvas {
        id: minMaxCanvas
        anchors.fill: parent
        z: 10

        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()

        property var data: sparklineCanvas.data
        onDataChanged: requestPaint()

        onPaint: {
            var ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)
            var d = data
            if (!d || d.length < 2) return

            var dataMin = d[0], dataMax = d[0]
            for (var i = 1; i < d.length; i++) {
                if (d[i] < dataMin) dataMin = d[i]
                if (d[i] > dataMax) dataMax = d[i]
            }

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

            drawTick(dataMin, "#5B8DD9")
            drawTick(dataMax, "#C84040")
        }
    }

    Shape {
        anchors.fill: parent

        ShapePath {
            strokeColor: "#2A2A2A"; fillColor: "transparent"
            strokeWidth: root.rateStroke; capStyle: ShapePath.FlatCap
            PathAngleArc {
                centerX: root.width / 2; centerY: root.height / 2
                radiusX: root.outerRadius; radiusY: root.outerRadius
                startAngle: root.arcStartAngle; sweepAngle: root.arcSweepAngle
            }
        }

        ShapePath {
            id: rateFillPath
            strokeColor: root.rateColor(root.rainRate); fillColor: "transparent"
            strokeWidth: root.rateStroke; capStyle: ShapePath.FlatCap
            property real animatedSweep: 0
            Behavior on animatedSweep { SmoothedAnimation { velocity: 200 } }
            Component.onCompleted: animatedSweep = root.rateTargetSweep
            PathAngleArc {
                centerX: root.width / 2; centerY: root.height / 2
                radiusX: root.outerRadius; radiusY: root.outerRadius
                startAngle: root.arcStartAngle; sweepAngle: rateFillPath.animatedSweep
            }
        }

        ShapePath {
            strokeColor: "#2A2A2A"; fillColor: "transparent"
            strokeWidth: root.dailyStroke; capStyle: ShapePath.FlatCap
            PathAngleArc {
                centerX: root.width / 2; centerY: root.height / 2
                radiusX: root.innerRadius; radiusY: root.innerRadius
                startAngle: root.arcStartAngle; sweepAngle: root.arcSweepAngle
            }
        }

        ShapePath {
            id: dailyFillPath
            strokeColor: "#C8A000"; fillColor: "transparent"
            strokeWidth: root.dailyStroke; capStyle: ShapePath.FlatCap
            property real animatedSweep: 0
            Behavior on animatedSweep { SmoothedAnimation { velocity: 200 } }
            Component.onCompleted: animatedSweep = root.dailyTargetSweep
            PathAngleArc {
                centerX: root.width / 2; centerY: root.height / 2
                radiusX: root.innerRadius; radiusY: root.innerRadius
                startAngle: root.arcStartAngle; sweepAngle: dailyFillPath.animatedSweep
            }
        }
    }

    Binding { target: rateFillPath;  property: "animatedSweep"; value: root.rateTargetSweep  }
    Binding { target: dailyFillPath; property: "animatedSweep"; value: root.dailyTargetSweep }

    Column {
        anchors.centerIn: parent
        width: root.innerRadius * 1.4
        spacing: 0

        Text {
            width: parent.width; text: "Rain Rate"; color: "#C8A000"
            font.pixelSize: Math.min(root.width, root.height) * 0.09
            horizontalAlignment: Text.AlignHCenter
            fontSizeMode: Text.HorizontalFit; minimumPixelSize: 8
        }
        Text {
            width: parent.width; text: root.rainRate.toFixed(2); color: "#C8A000"
            font.pixelSize: Math.min(root.width, root.height) * 0.18; font.bold: true
            horizontalAlignment: Text.AlignHCenter
            fontSizeMode: Text.HorizontalFit; minimumPixelSize: 10
        }
        Text {
            width: parent.width; text: "in/hr"; color: "#8A7A00"
            font.pixelSize: Math.min(root.width, root.height) * 0.075
            horizontalAlignment: Text.AlignHCenter
        }
        Text {
            width: parent.width; text: "Daily: " + root.dailyTotal.toFixed(2) + " in"; color: "#C8A000"
            font.pixelSize: Math.min(root.width, root.height) * 0.065
            horizontalAlignment: Text.AlignHCenter
            fontSizeMode: Text.HorizontalFit; minimumPixelSize: 8
        }
    }
}
