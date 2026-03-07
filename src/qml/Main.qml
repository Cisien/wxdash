import QtQuick
import QtQuick.Layouts

Window {
    id: root
    width: 1280
    height: 800
    visible: true
    title: "wxdash"
    color: "#1A1A1A"

    // Kiosk mode from C++ context property
    visibility: kioskMode ? Window.FullScreen : Window.Windowed
    flags: kioskMode ? Qt.Window | Qt.FramelessWindowHint : Qt.Window

    // F11 toggles fullscreen at runtime (KIOSK-01)
    Item {
        anchors.fill: parent
        focus: true
        Keys.onPressed: (event) => {
            if (event.key === Qt.Key_F11) {
                if (root.visibility === Window.FullScreen) {
                    root.visibility = Window.Windowed
                    root.flags = Qt.Window
                } else {
                    root.visibility = Window.FullScreen
                    root.flags = Qt.Window | Qt.FramelessWindowHint
                }
            }
        }
    }

    DashboardGrid {
        anchors.fill: parent
    }
}
