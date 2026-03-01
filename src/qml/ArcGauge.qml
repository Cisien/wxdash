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

    // Track arc (full 290-degree background)
    Shape {
        anchors.fill: parent

        ShapePath {
            strokeColor: "#2A2A2A"
            fillColor: "transparent"
            strokeWidth: root.strokeWidth
            capStyle: ShapePath.RoundCap

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
            capStyle: ShapePath.RoundCap

            // animatedSweep lives on the ShapePath to avoid binding pitfall
            property real animatedSweep: 0

            Behavior on animatedSweep {
                SmoothedAnimation {
                    velocity: 200
                }
            }

            // Keep animatedSweep chasing targetSweep
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

    // Label text (gauge title, positioned in upper third)
    Text {
        id: labelText
        anchors.horizontalCenter: parent.horizontalCenter
        y: parent.height * 0.32
        text: root.label
        color: "#C8A000"
        font.pixelSize: Math.min(root.width, root.height) * 0.09
        font.bold: false
        horizontalAlignment: Text.AlignHCenter
    }

    // Value + unit text (centered)
    Text {
        id: valueText
        anchors.centerIn: parent
        text: root.value.toFixed(root.decimals) + (root.unit !== "" ? " " + root.unit : "")
        color: "#C8A000"
        font.pixelSize: Math.min(root.width, root.height) * 0.18
        font.bold: true
        horizontalAlignment: Text.AlignHCenter
    }

    // Secondary text (optional, below value)
    Text {
        id: secondaryTextItem
        anchors.horizontalCenter: parent.horizontalCenter
        y: valueText.y + valueText.height + (root.height * 0.02)
        visible: root.secondaryText !== ""
        text: root.secondaryLabel !== "" ? root.secondaryLabel + ": " + root.secondaryText : root.secondaryText
        color: "#C8A000"
        font.pixelSize: Math.min(root.width, root.height) * 0.08
        horizontalAlignment: Text.AlignHCenter
    }
}
