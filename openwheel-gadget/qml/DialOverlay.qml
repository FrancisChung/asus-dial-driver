// openwheel-gadget/qml/DialOverlay.qml
import QtQuick
import QtQuick.Window

Window {
    id: overlayWindow
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.Tool
    color: "transparent"
    width: 320
    height: 320
    visible: dialController.menuOpen || hudTimer.running
    x: (Screen.width - width) / 2
    y: (Screen.height - height) / 2

    RadialMenu {
        anchors.fill: parent
        visible: dialController.menuOpen
    }

    Hud {
        id: hud
        anchors.fill: parent
        visible: !dialController.menuOpen && hudTimer.running
    }

    Timer {
        id: hudTimer
        interval: 1500
    }

    Connections {
        target: dialController
        function onHudRequested(iconName, valueLabel) {
            hud.iconName = iconName
            hud.valueLabel = valueLabel
            hudTimer.restart()
        }
    }
}
