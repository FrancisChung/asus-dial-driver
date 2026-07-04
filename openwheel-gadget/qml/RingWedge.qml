// openwheel-gadget/qml/RingWedge.qml
import QtQuick
import QtQuick.Shapes

Item {
    id: root
    property real centerAngle: 0
    property real spanAngle: 2 * Math.PI / 7
    property real innerRadius: 40
    property real outerRadius: 100
    property color fillColor: "#1a1a1a"
    property string iconId: ""
    property color iconColor: "#C9A87C"
    property real iconSize: 28

    readonly property real startAngle: root.centerAngle - root.spanAngle / 2
    readonly property real endAngle: root.centerAngle + root.spanAngle / 2
    readonly property real cx: width / 2
    readonly property real cy: height / 2
    readonly property real startDeg: root.startAngle * 180 / Math.PI
    readonly property real spanDeg: root.spanAngle * 180 / Math.PI

    function wedgePathData() {
        // Segment count scales with the actual angle swept, not a fixed
        // number — a fixed count looks fine for a small wedge slice but
        // produces a visibly faceted "annular sectors" look for a full
        // circle (e.g. CompactDial's background ring), which sweeps a much
        // larger angle through the same number of segments otherwise.
        var segmentsPerFullCircle = 90
        var segments = Math.max(3, Math.ceil(root.spanAngle / (2 * Math.PI) * segmentsPerFullCircle))
        var d = "M " + (root.cx + root.innerRadius * Math.cos(root.startAngle)) + "," + (root.cy + root.innerRadius * Math.sin(root.startAngle))
        d += " L " + (root.cx + root.outerRadius * Math.cos(root.startAngle)) + "," + (root.cy + root.outerRadius * Math.sin(root.startAngle))
        for (var i = 1; i <= segments; i++) {
            var a = root.startAngle + root.spanAngle * i / segments
            d += " L " + (root.cx + root.outerRadius * Math.cos(a)) + "," + (root.cy + root.outerRadius * Math.sin(a))
        }
        for (var j = segments; j >= 0; j--) {
            var a2 = root.startAngle + root.spanAngle * j / segments
            d += " L " + (root.cx + root.innerRadius * Math.cos(a2)) + "," + (root.cy + root.innerRadius * Math.sin(a2))
        }
        d += " Z"
        return d
    }

    Shape {
        anchors.fill: parent
        ShapePath {
            fillColor: root.fillColor
            strokeColor: "transparent"
            PathSvg { path: root.wedgePathData() }
        }
    }

    DialIcon {
        visible: root.iconId !== ""
        width: root.iconSize
        height: root.iconSize
        iconId: root.iconId
        color: root.iconColor
        x: root.cx + (root.innerRadius + root.outerRadius) / 2 * Math.cos(root.centerAngle) - width / 2
        y: root.cy + (root.innerRadius + root.outerRadius) / 2 * Math.sin(root.centerAngle) - height / 2
    }
}
