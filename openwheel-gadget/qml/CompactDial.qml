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
    // at the top. A small floor keeps the fill from fully vanishing at 0%, so
    // there's always a visible sliver (and the icon's background, see below,
    // stays consistently on the filled color rather than flipping to the
    // empty background right at 0%).
    readonly property real minFillSpanAngle: 2 * Math.PI * 0.03
    readonly property real fillSpanAngle: root.valuePercent >= 0
        ? Math.max(root.minFillSpanAngle, 2 * Math.PI * (root.valuePercent / 100))
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
        iconId: ""
    }

    // Drawn separately from the fill wedge above (not as that wedge's icon)
    // so it stays fixed at the top regardless of how far the fill has grown,
    // rather than riding the fill arc's midpoint around the ring.
    DialIcon {
        width: 22
        height: 22
        iconId: root.iconName
        color: "#1a1a1a"
        x: parent.width / 2 + (48 + 80) / 2 * Math.cos(-Math.PI / 2) - width / 2
        y: parent.height / 2 + (48 + 80) / 2 * Math.sin(-Math.PI / 2) - height / 2
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
