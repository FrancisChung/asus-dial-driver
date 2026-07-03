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

    readonly property real startAngle: root.centerAngle - root.spanAngle / 2
    readonly property real endAngle: root.centerAngle + root.spanAngle / 2
    readonly property real cx: width / 2
    readonly property real cy: height / 2
    readonly property real startDeg: root.startAngle * 180 / Math.PI
    readonly property real spanDeg: root.spanAngle * 180 / Math.PI

    Shape {
        anchors.fill: parent
        ShapePath {
            fillColor: root.fillColor
            strokeColor: "transparent"
            startX: root.cx + root.innerRadius * Math.cos(root.startAngle)
            startY: root.cy + root.innerRadius * Math.sin(root.startAngle)
            PathLine {
                x: root.cx + root.outerRadius * Math.cos(root.startAngle)
                y: root.cy + root.outerRadius * Math.sin(root.startAngle)
            }
            PathAngleArc {
                centerX: root.cx; centerY: root.cy
                radiusX: root.outerRadius; radiusY: root.outerRadius
                startAngle: root.startDeg
                sweepAngle: root.spanDeg
            }
            PathLine {
                x: root.cx + root.innerRadius * Math.cos(root.endAngle)
                y: root.cy + root.innerRadius * Math.sin(root.endAngle)
            }
            PathAngleArc {
                centerX: root.cx; centerY: root.cy
                radiusX: root.innerRadius; radiusY: root.innerRadius
                startAngle: root.startDeg + root.spanDeg
                sweepAngle: -root.spanDeg
            }
        }
    }

    DialIcon {
        visible: root.iconId !== ""
        width: 28
        height: 28
        iconId: root.iconId
        color: root.iconColor
        x: root.cx + (root.innerRadius + root.outerRadius) / 2 * Math.cos(root.centerAngle) - width / 2
        y: root.cy + (root.innerRadius + root.outerRadius) / 2 * Math.sin(root.centerAngle) - height / 2
    }
}
