import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: root
    width: 400
    height: 300
    visible: true
    title: qsTr("Hello QML WebAssembly")

    Column {
        anchors.centerIn: parent
        spacing: 16

        Label {
            text: "Hello from QML on Web!"
            font.pixelSize: 24
        }

        Button {
            text: "Click me"
            onClicked: console.log("Button clicked in WebAssembly!")
        }
    }
}
