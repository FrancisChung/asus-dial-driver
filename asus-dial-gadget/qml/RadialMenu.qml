// asus-dial-gadget/qml/RadialMenu.qml
import QtQuick

Item {
    id: root
    readonly property int count: dialController.functionCount()

    Repeater {
        model: root.count
        delegate: Rectangle {
            required property int index
            width: 64
            height: 64
            radius: width / 2
            color: index === dialController.highlightedIndex ? "#C9A87C" : "#1a1a1a"
            border.color: "#C9A87C"
            border.width: 1.5
            opacity: {
                var _menuOpenTrigger = dialController.menuOpen
                return dialController.isAvailableAt(index) ? 1.0 : 0.35
            }
            x: root.width / 2
               + (root.width / 2 - 40) * Math.cos(2 * Math.PI * index / root.count - Math.PI / 2)
               - width / 2
            y: root.height / 2
               + (root.height / 2 - 40) * Math.sin(2 * Math.PI * index / root.count - Math.PI / 2)
               - height / 2

            DialIcon {
                anchors.centerIn: parent
                width: 28
                height: 28
                iconId: dialController.iconNameAt(index)
                color: index === dialController.highlightedIndex ? "#1a1a1a" : "#C9A87C"
            }
        }
    }

    CenterLogo {
        anchors.centerIn: parent
    }
}
