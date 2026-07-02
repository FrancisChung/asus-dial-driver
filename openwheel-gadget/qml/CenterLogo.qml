// openwheel-gadget/qml/CenterLogo.qml
import QtQuick

Item {
    id: root

    Text {
        anchors.centerIn: parent
        text: "<b>ASUS</b> Dial"
        textFormat: Text.StyledText
        color: "#C9A87C"
        font.pixelSize: 22
        font.letterSpacing: 1
        horizontalAlignment: Text.AlignHCenter
    }
}
