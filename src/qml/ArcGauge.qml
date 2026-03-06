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

    SparklineCanvas {
        anchors.fill: parent
        z: -1
        data: root.sparklineData
        rangeMin: root.minValue
        rangeMax: root.maxValue
    }

    MinMaxTicks {
        anchors.fill: parent
        z: 10
        data: root.sparklineData
        rangeMin: root.minValue
        rangeMax: root.maxValue
        arcRadius: (Math.min(root.width, root.height) / 2) - (root.strokeWidth / 2)
        arcStrokeWidth: root.strokeWidth
        arcStartAngle: root.arcStartAngle
        arcSweepAngle: root.arcSweepAngle
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
