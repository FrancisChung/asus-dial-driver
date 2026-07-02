// openwheel-gadget/qml/DialIcon.qml
import QtQuick
import QtQuick.Shapes

Item {
    id: root
    property string iconId: ""
    property color color: "#C9A87C"

    // Volume: speaker body + cone + two soundwave arcs
    Item {
        visible: root.iconId === "audio-volume-high"
        anchors.fill: parent

        Shape {
            anchors.fill: parent
            ShapePath {
                strokeColor: root.color
                strokeWidth: 1.6
                fillColor: "transparent"
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                startX: 4; startY: 11
                PathLine { x: 9; y: 11 }
                PathLine { x: 15; y: 6 }
                PathLine { x: 15; y: 22 }
                PathLine { x: 9; y: 17 }
                PathLine { x: 4; y: 17 }
                PathLine { x: 4; y: 11 }
            }
            ShapePath {
                strokeColor: root.color
                strokeWidth: 1.6
                fillColor: "transparent"
                capStyle: ShapePath.RoundCap
                startX: 20; startY: 10
                PathAngleArc { centerX: 18; centerY: 14; radiusX: 4; radiusY: 4; startAngle: -40; sweepAngle: 80 }
            }
            ShapePath {
                strokeColor: root.color
                strokeWidth: 1.6
                fillColor: "transparent"
                capStyle: ShapePath.RoundCap
                startX: 23; startY: 7
                PathAngleArc { centerX: 18; centerY: 14; radiusX: 7; radiusY: 7; startAngle: -40; sweepAngle: 80 }
            }
        }
    }

    // Brightness: circle + 8 radiating rays
    Item {
        visible: root.iconId === "display-brightness"
        anchors.fill: parent

        Shape {
            anchors.fill: parent
            ShapePath {
                strokeColor: root.color
                strokeWidth: 1.6
                fillColor: "transparent"
                startX: 19; startY: 14
                PathAngleArc { centerX: 14; centerY: 14; radiusX: 5; radiusY: 5; startAngle: 0; sweepAngle: 360 }
            }
        }

        Repeater {
            model: 8
            delegate: Item {
                required property int index
                anchors.fill: parent
                rotation: index * 45

                Rectangle {
                    anchors.horizontalCenter: parent.horizontalCenter
                    y: 1
                    width: 1.6
                    height: 4
                    radius: 0.8
                    color: root.color
                }
            }
        }
    }

    // Scroll: stacked up/down chevrons
    Item {
        visible: root.iconId === "input-mouse"
        anchors.fill: parent

        Shape {
            anchors.fill: parent
            ShapePath {
                strokeColor: root.color
                strokeWidth: 1.8
                fillColor: "transparent"
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                startX: 8; startY: 11
                PathLine { x: 14; y: 5 }
                PathLine { x: 20; y: 11 }
            }
            ShapePath {
                strokeColor: root.color
                strokeWidth: 1.8
                fillColor: "transparent"
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                startX: 8; startY: 17
                PathLine { x: 14; y: 23 }
                PathLine { x: 20; y: 17 }
            }
        }
    }

    // Media: play triangle + pause bars
    Item {
        visible: root.iconId === "media-playback-start"
        anchors.fill: parent

        Shape {
            anchors.fill: parent
            ShapePath {
                strokeColor: root.color
                strokeWidth: 1.6
                fillColor: "transparent"
                joinStyle: ShapePath.RoundJoin
                startX: 7; startY: 9
                PathLine { x: 7; y: 19 }
                PathLine { x: 16; y: 14 }
                PathLine { x: 7; y: 9 }
            }
        }
        Rectangle { x: 19; y: 9; width: 2.2; height: 10; radius: 1; color: root.color }
        Rectangle { x: 23; y: 9; width: 2.2; height: 10; radius: 1; color: root.color }
    }
}
