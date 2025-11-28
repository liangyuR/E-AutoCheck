import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import EAutoCheck 1.0

ApplicationWindow {
    id: root
    width: 540
    height: 640
    visible: true
    title: qsTr("Self Check Demo")
    property var selfCheck: EAutoCheck.SelfCheck

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        Label {
            text: qsTr("充电箱自检调试")
            font.pixelSize: 24
            Layout.bottomMargin: 12
        }

        TextField {
            id: deviceIdInput
            Layout.fillWidth: true
            placeholderText: qsTr("设备 ID，例如 CHG-001")
            text: "CHG-001"
        }

        Button {
            Layout.fillWidth: true
            text: selfCheck.isChecking ? qsTr("自检中...") : qsTr("触发充电箱自检")
            enabled: !selfCheck.isChecking
            onClicked: {
                const trimmedId = deviceIdInput.text.trim();
                if (!trimmedId.length)
                    return;
                const reqId = selfCheck.triggerChargerBox(trimmedId);
                lastRequestLabel.text = qsTr("Request ID: ") + reqId;
            }
        }

        Label {
            id: statusLabel
            Layout.fillWidth: true
            text: selfCheck.isChecking ?
                      qsTr("状态：自检进行中...") :
                      qsTr("最近状态：") + selfCheck.lastOverallStatus()
        }

        Label {
            id: lastRequestLabel
            Layout.fillWidth: true
            text: qsTr("Request ID: -")
            color: "#666666"
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 8
            color: "#f5f5f5"
            border.color: "#dcdcdc"

            ListView {
                id: moduleList
                anchors.fill: parent
                anchors.margins: 12
                spacing: 8
                clip: true
                model: selfCheck.lastModules()

                delegate: Rectangle {
                    width: moduleList.width
                    height: 64
                    radius: 6
                    color: "#ffffff"
                    border.color: modelData.status === "warn" ? "#ffb74d"
                                 : modelData.status === "error" ? "#ef5350"
                                                                : "#e0e0e0"

                    Column {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 4

                        Text {
                            text: modelData.name + " (" + modelData.status + ")"
                            font.pixelSize: 16
                        }

                        Text {
                            text: modelData.message
                            font.pixelSize: 13
                            color: "#555555"
                            elide: Text.ElideRight
                        }
                    }
                }
            }

            Label {
                anchors.centerIn: parent
                visible: moduleList.count === 0
                text: qsTr("暂无自检结果，点击上方按钮触发一次。")
                color: "#888888"
            }
        }
    }
}
