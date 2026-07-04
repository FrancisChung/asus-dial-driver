// openwheel-gadget/qml/CompactDial.qml
import QtQuick

Item {
    id: root
    property string iconName: ""
    property string valueLabel: ""
    // 0-100 for functions with a meaningful level (Volume, Brightness); -1 means
    // "no percentage" (Scroll, Media), which keeps the original fixed-size arc.
    property int valuePercent: -1

    // When there's a real percentage, the highlighted arc starts at the top
    // (12 o'clock) and sweeps clockwise proportionally to the value, like a
    // fill gauge. Otherwise it keeps the original fixed 60-degree arc centered
    // at the top.
    readonly property real fillSpanAngle: root.valuePercent >= 0
        ? 2 * Math.PI * (root.valuePercent / 100)
        : Math.PI / 3
    readonly property real fillCenterAngle: root.valuePercent >= 0
        ? -Math.PI / 2 + root.fillSpanAngle / 2
        : -Math.PI / 2

    RingWedge {
        anchors.centerIn: parent
        width: 160
        height: 160
        centerAngle: 0
        spanAngle: 2 * Math.PI
        innerRadius: 48
        outerRadius: 80
        fillColor: "#1a1a1a"
        iconId: ""
    }

    RingWedge {
        anchors.centerIn: parent
        width: 160
        height: 160
        centerAngle: root.fillCenterAngle
        spanAngle: root.fillSpanAngle
        innerRadius: 48
        outerRadius: 80
        fillColor: "#C9A87C"
        iconId: root.iconName
        iconColor: "#1a1a1a"
        iconSize: 22
    }

    Text {
        anchors.centerIn: parent
        width: 90
        text: root.valueLabel
        color: "#C9A87C"
        font.pixelSize: 20
        font.letterSpacing: 0.5
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
    }
}
