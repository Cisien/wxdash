import QtQuick
import QtQuick.Shapes

Item {
    id: root

    property real windSpeed: 0.0
    property real windGust: 0.0
    property var sparklineData: []
    property real sparklineMin: NaN
    property real sparklineMax: NaN
    property var gustSparklineData: []
    property real gustSparklineMin: NaN
    property real gustSparklineMax: NaN
    property color arcColor: "#C8A000"

    readonly property real arcStartAngle: 125
    readonly property real arcSweepAngle: 290
    readonly property real totalStrokeWidth: Math.min(width, height) * 0.08
    readonly property real speedStroke: totalStrokeWidth * 0.6
    readonly property real gustStroke: totalStrokeWidth * 0.4
    readonly property real outerRadius: (Math.min(width, height) / 2) - (speedStroke / 2)
    readonly property real innerRadius: outerRadius - (speedStroke / 2) - (gustStroke / 2)
    readonly property real speedMax: 60.0
    readonly property real gustMax: 60.0
    readonly property real speedTargetSweep: arcSweepAngle * Math.max(0, Math.min(1, windSpeed / speedMax))
    readonly property real gustTargetSweep: arcSweepAngle * Math.max(0, Math.min(1, windGust / gustMax))

    SparklineCanvas {
        anchors.fill: parent
        z: -1
        data: root.sparklineData
        rangeMin: 0
        rangeMax: root.speedMax
    }

    MinMaxTicks {
        anchors.fill: parent
        z: 10
        trueMin: root.sparklineMin
        trueMax: root.sparklineMax
        rangeMin: 0
        rangeMax: root.speedMax
        arcRadius: root.outerRadius
        arcStrokeWidth: root.speedStroke
        arcStartAngle: root.arcStartAngle
        arcSweepAngle: root.arcSweepAngle
    }

    MinMaxTicks {
        anchors.fill: parent
        z: 10
        trueMin: root.gustSparklineMin
        trueMax: root.gustSparklineMax
        rangeMin: 0
        rangeMax: root.gustMax
        arcRadius: root.innerRadius
        arcStrokeWidth: root.gustStroke
        arcStartAngle: root.arcStartAngle
        arcSweepAngle: root.arcSweepAngle
    }

    Shape {
        anchors.fill: parent

        ShapePath {
            strokeColor: "#2A2A2A"; fillColor: "transparent"
            strokeWidth: root.speedStroke; capStyle: ShapePath.FlatCap
            PathAngleArc {
                centerX: root.width / 2; centerY: root.height / 2
                radiusX: root.outerRadius; radiusY: root.outerRadius
                startAngle: root.arcStartAngle; sweepAngle: root.arcSweepAngle
            }
        }

        ShapePath {
            id: speedFillPath
            strokeColor: root.arcColor; fillColor: "transparent"
            strokeWidth: root.speedStroke; capStyle: ShapePath.FlatCap
            property real animatedSweep: 0
            Behavior on animatedSweep { SmoothedAnimation { velocity: 200 } }
            Component.onCompleted: animatedSweep = root.speedTargetSweep
            PathAngleArc {
                centerX: root.width / 2; centerY: root.height / 2
                radiusX: root.outerRadius; radiusY: root.outerRadius
                startAngle: root.arcStartAngle; sweepAngle: speedFillPath.animatedSweep
            }
        }

        ShapePath {
            strokeColor: "#2A2A2A"; fillColor: "transparent"
            strokeWidth: root.gustStroke; capStyle: ShapePath.FlatCap
            PathAngleArc {
                centerX: root.width / 2; centerY: root.height / 2
                radiusX: root.innerRadius; radiusY: root.innerRadius
                startAngle: root.arcStartAngle; sweepAngle: root.arcSweepAngle
            }
        }

        ShapePath {
            id: gustFillPath
            strokeColor: "#C8A000"; fillColor: "transparent"
            strokeWidth: root.gustStroke; capStyle: ShapePath.FlatCap
            property real animatedSweep: 0
            Behavior on animatedSweep { SmoothedAnimation { velocity: 200 } }
            Component.onCompleted: animatedSweep = root.gustTargetSweep
            PathAngleArc {
                centerX: root.width / 2; centerY: root.height / 2
                radiusX: root.innerRadius; radiusY: root.innerRadius
                startAngle: root.arcStartAngle; sweepAngle: gustFillPath.animatedSweep
            }
        }
    }

    Binding { target: speedFillPath; property: "animatedSweep"; value: root.speedTargetSweep }
    Binding { target: gustFillPath; property: "animatedSweep"; value: root.gustTargetSweep }

    readonly property real innerRadiusText: (Math.min(width, height) / 2) - totalStrokeWidth

    Column {
        anchors.centerIn: parent
        width: root.innerRadiusText * 1.4
        spacing: 0

        Text {
            width: parent.width; text: "Wind"; color: "#C8A000"
            font.pixelSize: Math.min(root.width, root.height) * 0.09
            horizontalAlignment: Text.AlignHCenter
            fontSizeMode: Text.HorizontalFit; minimumPixelSize: 8
        }
        Text {
            width: parent.width; text: root.windSpeed.toFixed(1); color: "#C8A000"
            font.pixelSize: Math.min(root.width, root.height) * 0.18; font.bold: true
            horizontalAlignment: Text.AlignHCenter
            fontSizeMode: Text.HorizontalFit; minimumPixelSize: 10
        }
        Text {
            width: parent.width; text: "mph"; color: "#8A7A00"
            font.pixelSize: Math.min(root.width, root.height) * 0.075
            horizontalAlignment: Text.AlignHCenter
        }
        Text {
            width: parent.width; text: "Gust: " + root.windGust.toFixed(1) + " mph"; color: "#C8A000"
            font.pixelSize: Math.min(root.width, root.height) * 0.065
            horizontalAlignment: Text.AlignHCenter
            fontSizeMode: Text.HorizontalFit; minimumPixelSize: 8
        }
    }
}
