// openwheel-gadget/qml/Hud.qml
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
        color: "#222222"
        opacity: 0.85

        Text {
            anchors.centerIn: parent
            text: root.valueLabel
            color: "white"
            font.pixelSize: 20
        }
    }
}
