// openwheel-gadget/qml/RadialMenu.qml
import QtQuick

Item {
    id: root
    readonly property int totalWedgeCount: 7
    readonly property int realCount: dialController.functionCount()

    Repeater {
        model: root.totalWedgeCount
        delegate: RingWedge {
            required property int index
            anchors.fill: parent
            centerAngle: 2 * Math.PI * index / root.totalWedgeCount - Math.PI / 2
            spanAngle: 2 * Math.PI / root.totalWedgeCount
            innerRadius: 60
            outerRadius: 150
            fillColor: index < root.realCount && index === dialController.highlightedIndex
                       ? "#C9A87C"
                       : "#1a1a1a"
            iconId: index < root.realCount ? dialController.iconNameAt(index) : ""
            iconColor: index < root.realCount && index === dialController.highlightedIndex
                       ? "#1a1a1a"
                       : "#C9A87C"
            opacity: {
                var _menuOpenTrigger = dialController.menuOpen
                if (index >= root.realCount) return 0.35
                return dialController.isAvailableAt(index) ? 1.0 : 0.35
            }
        }
    }

    CenterLogo {
        anchors.centerIn: parent
    }
}
