// asus-dial-gadget/qml/DialOverlay.qml
import QtQuick
import QtQuick.Window

Window {
    id: overlayWindow
    property bool hudVisible: false

    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.Tool
    color: "transparent"
    width: 320
    height: 320
    visible: dialController.menuOpen || hudVisible
    x: (Screen.width - width) / 2
    y: (Screen.height - height) / 2

    RadialMenu {
        anchors.fill: parent
        visible: dialController.menuOpen
    }

    CompactDial {
        id: compactDial
        anchors.fill: parent
        visible: !dialController.menuOpen && overlayWindow.hudVisible
    }

    Timer {
        id: hudTimer
        interval: 1500
        onTriggered: overlayWindow.hudVisible = false
    }

    Connections {
        target: dialController
        function onHudRequested(iconName, valueLabel, valuePercent, direction) {
            compactDial.iconName = iconName
            compactDial.valueLabel = valueLabel
            compactDial.valuePercent = valuePercent
            compactDial.direction = direction
            overlayWindow.hudVisible = true
            hudTimer.restart()
        }
    }
}
