// asus-dial-gadget/qml/Hud.qml
import QtQuick

Item {
    id: root
    property string iconName: ""
    property string valueLabel: ""

    Rectangle {
        anchors.centerIn: parent
        width: 160
        height: 60
        radius: 12
        color: "#1a1a1a"
        border.color: "#C9A87C"
        border.width: 1.5
        opacity: 0.92

        Text {
            anchors.centerIn: parent
            text: root.valueLabel
            color: "#C9A87C"
            font.pixelSize: 20
            font.letterSpacing: 0.5
        }
    }
}
