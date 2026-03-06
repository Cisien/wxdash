import QtQuick
import QtQuick.Shapes

Item {
    id: root

    property real rainRate:     0.0
    property real dailyTotal:   0.0
    property var sparklineData: []
    property real sparklineMin: NaN
    property real sparklineMax: NaN

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

    SparklineCanvas {
        anchors.fill: parent
        z: -1
        data: root.sparklineData
        rangeMin: 0
        rangeMax: root.rateMax
    }

    MinMaxTicks {
        anchors.fill: parent
        z: 10
        trueMin: root.sparklineMin
        trueMax: root.sparklineMax
        rangeMin: 0
        rangeMax: root.rateMax
        arcRadius: root.outerRadius
        arcStrokeWidth: root.rateStroke
        arcStartAngle: root.arcStartAngle
        arcSweepAngle: root.arcSweepAngle
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
