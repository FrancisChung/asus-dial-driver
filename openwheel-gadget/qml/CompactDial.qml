// openwheel-gadget/qml/CompactDial.qml
import QtQuick

Item {
    id: root
    property string iconName: ""
    property string valueLabel: ""

    RingWedge {
        anchors.centerIn: parent
        width: 100
        height: 100
        centerAngle: 0
        spanAngle: 2 * Math.PI
        innerRadius: 30
        outerRadius: 48
        fillColor: "#1a1a1a"
        iconId: ""
    }

    RingWedge {
        anchors.centerIn: parent
        width: 100
        height: 100
        centerAngle: -Math.PI / 2
        spanAngle: Math.PI / 3
        innerRadius: 30
        outerRadius: 48
        fillColor: "#C9A87C"
        iconId: root.iconName
        iconColor: "#1a1a1a"
    }

    Text {
        anchors.centerIn: parent
        text: root.valueLabel
        color: "#C9A87C"
        font.pixelSize: 16
        font.letterSpacing: 0.5
    }
}
