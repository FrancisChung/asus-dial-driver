// asus-dial-gadget/qml/CompactDial.qml
import QtQuick

Item {
    id: root
    property string iconName: ""
    property string valueLabel: ""
    // 0-100 for functions with a meaningful level (Volume, Brightness); -1 means
    // "no percentage" (Scroll, Media) — those use the last rotation direction
    // instead (see below).
    property int valuePercent: -1
    // Sign of the last rotate tick's direction (+1/-1), 0 if not applicable
    // (e.g. the menu-close confirmation, which isn't a rotation).
    property int direction: 0

    // When there's a real percentage, the highlighted arc starts at the top
    // (12 o'clock) and sweeps clockwise proportionally to the value, like a
    // fill gauge. A small floor keeps the fill from fully vanishing at 0%, so
    // there's always a visible sliver (and the icon's background, see below,
    // stays consistently on the filled color rather than flipping to the
    // empty background right at 0%).
    //
    // Without a percentage (Scroll, Media), there's no absolute level to show
    // a fill amount for — instead the same highlighted-arc language shows
    // which way the dial was last turned: one side for one direction, the
    // other side for the other, centered at the top when idle/not applicable.
    readonly property real minFillSpanAngle: 2 * Math.PI * 0.03
    readonly property real directionalSpanAngle: Math.PI / 2
    readonly property real fillSpanAngle: root.valuePercent >= 0
        ? Math.max(root.minFillSpanAngle, 2 * Math.PI * (root.valuePercent / 100))
        : root.directionalSpanAngle
    readonly property real fillCenterAngle: {
        if (root.valuePercent >= 0) {
            return -Math.PI / 2 + root.fillSpanAngle / 2
        }
        if (root.direction > 0) {
            return 0            // 3 o'clock
        }
        if (root.direction < 0) {
            return Math.PI      // 9 o'clock
        }
        return -Math.PI / 2     // idle: centered at top
    }

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
    //
    // Volume and Brightness use DialIcon's PathAngleArc-based glyphs (speaker
    // soundwaves / brightness rays), which don't render cleanly at this small
    // size, so those two are left blank here rather than shown malformed;
    // Scroll and Media's straight-line glyphs are unaffected.
    DialIcon {
        width: 22
        height: 22
        iconId: (root.iconName === "audio-volume-high" || root.iconName === "display-brightness")
            ? "" : root.iconName
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
