// DeviceCard.qml
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15
import Qt5Compat.GraphicalEffects
import GUI

Control {
    id: root

    // 基础属性：充电桩 / 换电站都可以用
    property string deviceId: ""
    property string name: ""
    property string deviceType: ""      // "充电桩" / "换电站" / 其他
    property string statusText: ""      // "正常" / "警告" / "异常"
    property string statusLevel: "normal" // "normal" / "warning" / "error"
    property string lastCheck: ""       // 最近自检时间
    property string location: ""        // 位置描述
    property bool online: true          // 在线 / 离线
    property bool checking: false       // 是否正在自检

    signal selfCheckRequested(string deviceId)

    implicitWidth: 280
    implicitHeight: 180
    padding: AppLayout.marginMedium

    background: Rectangle {
        radius: AppLayout.radiusMedium
        color: AppTheme.backgroundSecondary
        border.width: 1
        border.color: root.statusLevel === "error" ? AppTheme.error
                        : root.statusLevel === "warning" ? AppTheme.warning
                        : AppTheme.borderLight
        
        layer.enabled: true
        layer.effect: DropShadow {
            horizontalOffset: 0
            verticalOffset: 2
            radius: 8
            samples: 16
            color: AppTheme.overlayLight
        }
    }

    contentItem: ColumnLayout {
        anchors.fill: parent
        spacing: AppLayout.spacingSmall

        // 顶部：名称 + 类型标签
        RowLayout {
            Layout.fillWidth: true

            Label {
                text: root.name.length ? root.name : root.deviceId
                font: AppFont.subtitle
                Layout.fillWidth: true
                elide: Label.ElideRight
                color: AppTheme.textPrimary
            }

            Rectangle {
                radius: AppLayout.radiusSmall
                height: 24
                color: AppTheme.backgroundTertiary
                border.width: 1
                border.color: AppTheme.borderLight
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                width: typeLabel.implicitWidth + AppLayout.marginSmall * 2

                Label {
                    id: typeLabel
                    anchors.centerIn: parent
                    text: root.deviceType
                    font: AppFont.caption
                    color: AppTheme.textSecondary
                }
            }
        }

        // 中部：在线状态 + 位置 + 总体状态
        RowLayout {
            Layout.fillWidth: true
            spacing: AppLayout.spacingMedium

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4

                Label {
                    text: root.online ? "在线" : "离线"
                    color: root.online ? AppTheme.success : AppTheme.textTertiary
                    font: AppFont.body
                }

                Label {
                    text: "位置: " + (root.location || "-")
                    font: AppFont.caption
                    color: AppTheme.textSecondary
                    elide: Label.ElideRight
                }
            }

            Rectangle {
                width: 12
                height: 12
                radius: 6
                Layout.alignment: Qt.AlignVCenter
                color: root.statusLevel === "error" ? AppTheme.error
                       : root.statusLevel === "warning" ? AppTheme.warning
                       : AppTheme.success
            }

            Label {
                text: root.statusText
                font: AppFont.body
                color: AppTheme.textPrimary
                Layout.alignment: Qt.AlignVCenter
            }
        }

        // 占位，把按钮压到底部
        Item {
            Layout.fillHeight: true
        }

        // 底部：最近自检时间 + 自检按钮
        RowLayout {
            Layout.fillWidth: true
            spacing: AppLayout.spacingSmall

            Label {
                text: "最近自检: " + (root.lastCheck || "从未")
                font: AppFont.caption
                color: AppTheme.textTertiary
                Layout.fillWidth: true
                elide: Label.ElideRight
            }

            Button {
                text: root.checking ? "自检中..." : "自检"
                enabled: !root.checking && root.online
                Layout.preferredWidth: 80
                Material.elevation: 2
                highlighted: !root.checking
                onClicked: root.selfCheckRequested(root.deviceId)
            }
        }
    }
}
