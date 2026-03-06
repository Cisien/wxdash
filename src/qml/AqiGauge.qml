import QtQuick
import QtQuick.Shapes

Item {
    id: root

    // Public API
    property real aqiValue:  0.0   // 0-500
    property real pm25Value: 0.0   // ug/m3
    property real pm10Value: 0.0   // ug/m3
    property color aqiColor: "#5CA85C"  // EPA color, bound from DashboardGrid
    property var sparklineData: []      // QVariantList for AQI sparkline

    // Arc geometry constants — match ArcGauge proportions
    readonly property real arcStartAngle: 125
    readonly property real arcSweepAngle: 290

    // Total stroke width matches ArcGauge
    readonly property real totalStrokeWidth: Math.min(width, height) * 0.08

    // Partition: AQI outer 1/2, PM2.5 middle 1/4, PM10 inner 1/4
    readonly property real aqiStroke:  totalStrokeWidth * 0.5
    readonly property real pm25Stroke: totalStrokeWidth * 0.25
    readonly property real pm10Stroke: totalStrokeWidth * 0.25

    // Center radius of each ring (accounting for half-strokes to avoid gaps/overlaps)
    readonly property real outerRadius:  (Math.min(width, height) / 2) - (aqiStroke / 2)
    readonly property real middleRadius: outerRadius  - (aqiStroke  / 2) - (pm25Stroke / 2)
    readonly property real innerRadius:  middleRadius - (pm25Stroke / 2) - (pm10Stroke / 2)

    // Sweep calculations (value to arc proportion)
    // AQI: 0-500 range
    readonly property real aqiTargetSweep:  arcSweepAngle * Math.max(0, Math.min(1, aqiValue  / 500.0))
    // PM2.5: 0-500 ug/m3 range
    readonly property real pm25TargetSweep: arcSweepAngle * Math.max(0, Math.min(1, pm25Value / 500.0))
    // PM10: 0-500 ug/m3 range
    readonly property real pm10TargetSweep: arcSweepAngle * Math.max(0, Math.min(1, pm10Value / 500.0))

    // Sparkline background overlay — AQI history rendered behind gauge arcs
    Canvas {
        id: sparklineCanvas
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        height: parent.height / 3      // lower third of cell
        z: -1                           // behind gauge arcs and text

        onWidthChanged:  requestPaint()
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

            // Use AQI 0-500 range as baseline, expand if data exceeds
            var minV = 0, maxV = 500
            for (var i = 0; i < count; i++) {
                if (data[i] < minV) minV = data[i]
                if (data[i] > maxV) maxV = data[i]
            }
            var range = maxV - minV
            if (range === 0) range = 1  // flat line — avoid div by zero

            ctx.beginPath()
            ctx.strokeStyle = "#5A4500"   // dim yellow, subdued against #1A1A1A background
            ctx.lineWidth = 1
            ctx.lineJoin = "round"

            var first = true
            for (var j = 0; j < count; j += stride) {
                var x = (j / (count - 1)) * width
                // Y: 0 = top, height = bottom. Map data so max is at top (10% padding)
                var y = height - ((data[j] - minV) / range) * height * 0.9
                if (first) { ctx.moveTo(x, y); first = false }
                else ctx.lineTo(x, y)
            }
            ctx.stroke()
        }
    }

    // Single Shape with 6 ShapePaths (3 tracks + 3 fills)
    Shape {
        anchors.fill: parent

        // 1. AQI track (outermost)
        ShapePath {
            strokeColor: "#2A2A2A"
            fillColor: "transparent"
            strokeWidth: root.aqiStroke
            capStyle: ShapePath.FlatCap
            PathAngleArc {
                centerX: root.width / 2; centerY: root.height / 2
                radiusX: root.outerRadius; radiusY: root.outerRadius
                startAngle: root.arcStartAngle; sweepAngle: root.arcSweepAngle
            }
        }

        // 2. AQI fill (outermost, EPA color, animated)
        ShapePath {
            id: aqiFillPath
            strokeColor: root.aqiColor
            fillColor: "transparent"
            strokeWidth: root.aqiStroke
            capStyle: ShapePath.FlatCap
            property real animatedSweep: 0
            Behavior on animatedSweep {
                SmoothedAnimation { velocity: 200 }
            }
            Component.onCompleted: animatedSweep = root.aqiTargetSweep
            PathAngleArc {
                centerX: root.width / 2; centerY: root.height / 2
                radiusX: root.outerRadius; radiusY: root.outerRadius
                startAngle: root.arcStartAngle
                sweepAngle: aqiFillPath.animatedSweep
            }
        }

        // 3. PM2.5 track (middle)
        ShapePath {
            strokeColor: "#2A2A2A"
            fillColor: "transparent"
            strokeWidth: root.pm25Stroke
            capStyle: ShapePath.FlatCap
            PathAngleArc {
                centerX: root.width / 2; centerY: root.height / 2
                radiusX: root.middleRadius; radiusY: root.middleRadius
                startAngle: root.arcStartAngle; sweepAngle: root.arcSweepAngle
            }
        }

        // 4. PM2.5 fill (middle, neutral yellow, animated)
        ShapePath {
            id: pm25FillPath
            strokeColor: "#C8A000"
            fillColor: "transparent"
            strokeWidth: root.pm25Stroke
            capStyle: ShapePath.FlatCap
            property real animatedSweep: 0
            Behavior on animatedSweep {
                SmoothedAnimation { velocity: 200 }
            }
            Component.onCompleted: animatedSweep = root.pm25TargetSweep
            PathAngleArc {
                centerX: root.width / 2; centerY: root.height / 2
                radiusX: root.middleRadius; radiusY: root.middleRadius
                startAngle: root.arcStartAngle
                sweepAngle: pm25FillPath.animatedSweep
            }
        }

        // 5. PM10 track (innermost)
        ShapePath {
            strokeColor: "#2A2A2A"
            fillColor: "transparent"
            strokeWidth: root.pm10Stroke
            capStyle: ShapePath.FlatCap
            PathAngleArc {
                centerX: root.width / 2; centerY: root.height / 2
                radiusX: root.innerRadius; radiusY: root.innerRadius
                startAngle: root.arcStartAngle; sweepAngle: root.arcSweepAngle
            }
        }

        // 6. PM10 fill (innermost, neutral yellow, animated)
        ShapePath {
            id: pm10FillPath
            strokeColor: "#C8A000"
            fillColor: "transparent"
            strokeWidth: root.pm10Stroke
            capStyle: ShapePath.FlatCap
            property real animatedSweep: 0
            Behavior on animatedSweep {
                SmoothedAnimation { velocity: 200 }
            }
            Component.onCompleted: animatedSweep = root.pm10TargetSweep
            PathAngleArc {
                centerX: root.width / 2; centerY: root.height / 2
                radiusX: root.innerRadius; radiusY: root.innerRadius
                startAngle: root.arcStartAngle
                sweepAngle: pm10FillPath.animatedSweep
            }
        }
    }

    // Bindings to drive animated sweeps (same pattern as ArcGauge)
    Binding { target: aqiFillPath;  property: "animatedSweep"; value: root.aqiTargetSweep  }
    Binding { target: pm25FillPath; property: "animatedSweep"; value: root.pm25TargetSweep }
    Binding { target: pm10FillPath; property: "animatedSweep"; value: root.pm10TargetSweep }

    // Min/max tick marks on outer (AQI) arc — computed inline to avoid extra sparklineToList calls
    Canvas {
        id: minMaxCanvas
        anchors.fill: parent
        z: 10

        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()

        property var histData: root.sparklineData
        onHistDataChanged: requestPaint()

        onPaint: {
            var ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)
            var d = histData
            if (!d || d.length < 2) return

            var sMin = d[0], sMax = d[0]
            for (var i = 1; i < d.length; i++) {
                if (d[i] < sMin) sMin = d[i]
                if (d[i] > sMax) sMax = d[i]
            }

            var cx = width / 2
            var cy = height / 2
            var r = root.outerRadius
            var halfStroke = root.aqiStroke / 2

            function drawTick(val, color) {
                var ratio = Math.max(0, Math.min(1, val / 500.0))
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

            drawTick(sMin, "#5B8DD9")
            drawTick(sMax, "#C84040")
        }
    }

    // Inner text layout — centered in gauge
    Column {
        anchors.centerIn: parent
        width: root.innerRadius * 1.4
        spacing: 0

        // Label
        Text {
            width: parent.width
            text: "AQI"
            color: "#C8A000"
            font.pixelSize: Math.min(root.width, root.height) * 0.09
            horizontalAlignment: Text.AlignHCenter
            fontSizeMode: Text.HorizontalFit
            minimumPixelSize: 8
        }

        // AQI value (large, prominent)
        Text {
            width: parent.width
            text: Math.round(root.aqiValue).toString()
            color: "#C8A000"
            font.pixelSize: Math.min(root.width, root.height) * 0.22
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
            fontSizeMode: Text.HorizontalFit
            minimumPixelSize: 10
        }
    }

    // PM2.5 sub-label — bottom left
    Text {
        anchors.bottom: parent.bottom
        anchors.bottomMargin: parent.height * 0.05
        anchors.left: parent.left
        anchors.leftMargin: parent.width * 0.02
        text: "PM2.5: " + root.pm25Value.toFixed(1)
        color: "#8A7A00"
        font.pixelSize: Math.min(root.width, root.height) * 0.06
    }

    // PM10 sub-label — bottom right
    Text {
        anchors.bottom: parent.bottom
        anchors.bottomMargin: parent.height * 0.05
        anchors.right: parent.right
        anchors.rightMargin: parent.width * 0.02
        text: "PM10: " + root.pm10Value.toFixed(1)
        color: "#8A7A00"
        font.pixelSize: Math.min(root.width, root.height) * 0.06
    }
}
