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
        centerAngle: root.fillCenterAngle
        spanAngle: root.fillSpanAngle
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
