import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

// 状态行组件
Pane {
    id: statusBar

    // 状态更新时间戳
    property string currentTime: Qt.formatDateTime(new Date(), "hh:mm:ss")

    Layout.fillWidth: true
    Layout.preferredHeight: 60
    Material.background: Material.surface

    // 定时器，每秒更新时间戳
    Timer {
        interval: 1000
        repeat: true
        running: true

        onTriggered: {
            currentTime = Qt.formatDateTime(new Date(), "hh:mm:ss");
        }
    }
    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        spacing: 16

        // 机械臂状态图标
        Rectangle {
            color: robotManager.connected ? Material.color(Material.Green, Material.Shade500) : Material.color(Material.Red, Material.Shade500)
            height: 12
            radius: 6
            width: 12

            BusyIndicator {
                anchors.centerIn: parent
                height: 8
                running: robotManager.connecting
                visible: robotManager.connecting
                width: 8
            }
        }

        // 机械臂状态文本
        Label {
            font.pixelSize: 12
            font.weight: Font.Medium
            text: {
                if (robotManager.connecting)
                    return "机械臂: 连接中...";
                else if (robotManager.connected)
                    return "机械臂: 已连接";
                else
                    return "机械臂: 未连接";
            }
        }

        // 分隔符
        Rectangle {
            color: Material.color(Material.Grey, Material.Shade300)
            height: 16
            width: 1
        }

        // 状态信息组
        RowLayout {
            spacing: 12
            visible: robotManager.connected

            // 电源状态
            RowLayout {
                spacing: 4

                Rectangle {
                    color: robotManager.poweredOn ? Material.color(Material.Green, Material.Shade500) : Material.color(Material.Grey, Material.Shade500)
                    height: 6
                    radius: 3
                    width: 6
                }
                Label {
                    color: Material.color(Material.Grey, Material.Shade500)
                    font.pixelSize: 11
                    text: "电源"
                }
                Label {
                    color: robotManager.poweredOn ? Material.color(Material.Green, Material.Shade500) : Material.color(Material.Grey, Material.Shade500)
                    font.pixelSize: 11
                    text: robotManager.poweredOn ? "已上电" : "未上电"
                }
            }

            // 使能状态
            RowLayout {
                spacing: 4

                Rectangle {
                    color: robotManager.enabled ? Material.color(Material.Green, Material.Shade500) : Material.color(Material.Grey, Material.Shade500)
                    height: 6
                    radius: 3
                    width: 6
                }
                Label {
                    color: Material.color(Material.Grey, Material.Shade500)
                    font.pixelSize: 11
                    text: "使能"
                }
                Label {
                    color: robotManager.enabled ? Material.color(Material.Green, Material.Shade500) : Material.color(Material.Grey, Material.Shade500)
                    font.pixelSize: 11
                    text: robotManager.enabled ? "已使能" : "未使能"
                }
            }

            // 错误状态
            RowLayout {
                spacing: 4

                Rectangle {
                    color: robotManager.error ? Material.color(Material.Red, Material.Shade500) : Material.color(Material.Green, Material.Shade500)
                    height: 6
                    radius: 3
                    width: 6
                }
                Label {
                    color: Material.color(Material.Grey, Material.Shade500)
                    font.pixelSize: 11
                    text: "错误"
                }
                Label {
                    color: robotManager.error ? Material.color(Material.Red, Material.Shade500) : Material.color(Material.Green, Material.Shade500)
                    font.pixelSize: 11
                    text: robotManager.error ? "有错误" : "正常"
                }
            }

            // 急停状态
            RowLayout {
                spacing: 4

                Rectangle {
                    color: robotManager.emergencyStop ? Material.color(Material.Red, Material.Shade500) : Material.color(Material.Green, Material.Shade500)
                    height: 6
                    radius: 3
                    width: 6
                }
                Label {
                    color: Material.color(Material.Grey, Material.Shade500)
                    font.pixelSize: 11
                    text: "急停"
                }
                Label {
                    color: robotManager.emergencyStop ? Material.color(Material.Red, Material.Shade500) : Material.color(Material.Green, Material.Shade500)
                    font.pixelSize: 11
                    text: robotManager.emergencyStop ? "已急停" : "正常"
                }
            }

            // 碰撞状态
            RowLayout {
                spacing: 4

                Rectangle {
                    color: robotManager.protectiveStop ? Material.color(Material.Red, Material.Shade500) : Material.color(Material.Green, Material.Shade500)
                    height: 6
                    radius: 3
                    width: 6
                }
                Label {
                    color: Material.color(Material.Grey, Material.Shade500)
                    font.pixelSize: 11
                    text: "碰撞"
                }
                Label {
                    color: robotManager.protectiveStop ? Material.color(Material.Red, Material.Shade500) : Material.color(Material.Green, Material.Shade500)
                    font.pixelSize: 11
                    text: robotManager.protectiveStop ? "检测到" : "正常"
                }
            }

            // 到位状态
            RowLayout {
                spacing: 4

                Rectangle {
                    color: robotManager.inpos ? Material.color(Material.Green, Material.Shade500) : Material.color(Material.Orange, Material.Shade500)
                    height: 6
                    radius: 3
                    width: 6
                }
                Label {
                    color: Material.color(Material.Grey, Material.Shade500)
                    font.pixelSize: 11
                    text: "到位"
                }
                Label {
                    color: robotManager.inpos ? Material.color(Material.Green, Material.Shade500) : Material.color(Material.Orange, Material.Shade500)
                    font.pixelSize: 11
                    text: robotManager.inpos ? "到位" : "未到位"
                }
            }
        }
        Item {
            Layout.fillWidth: true
        }

        // 右侧信息组
        RowLayout {
            spacing: 12

            // 运动倍率
            RowLayout {
                spacing: 4
                visible: robotManager.connected

                Label {
                    color: Material.color(Material.Grey, Material.Shade500)
                    font.pixelSize: 11
                    text: "倍率:"
                }
                Label {
                    font.pixelSize: 11
                    text: robotManager.rapidrate + "%"
                }
            }

            // 分隔符
            Rectangle {
                color: Material.color(Material.Grey, Material.Shade300)
                height: 16
                visible: robotManager.connected
                width: 1
            }

            // 时间戳
            Label {
                color: Material.color(Material.Grey, Material.Shade500)
                font.pixelSize: 11
                text: "更新时间: " + currentTime
            }
        }
    }
}
