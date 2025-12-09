import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 2.15
import GUI   // AppTheme / AppFont / AppLayout
import EAutoCheck 1.0

Item {
    id: root
    anchors.fill: parent

    // 使用 C++ PileModel（若没有数据可调用 loadDemo）
    Component.onCompleted: {
        if (PileModel.count === 0 && PileModel.loadDemo) {
            PileModel.loadDemo()
        }
    }

    // 文案 + 颜色的小工具函数
    function statusText(fault) {
        return fault ? qsTr("故障") : qsTr("正常")
    }

    function statusColor(fault) {
        return fault ? AppTheme.error : AppTheme.success
    }

    Rectangle {
        anchors.fill: parent
        color: AppTheme.backgroundSecondary

        // 显示 设备ID，类型，检查时间
        Rectangle {
            id: headerArea
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: headerLayout.implicitHeight + AppLayout.marginLarge * 2
            color: AppTheme.backgroundPrimary
            visible: PileModel.count > 0

            ColumnLayout {
                id: headerLayout
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: AppLayout.marginLarge
                spacing: AppLayout.spacingSmall

                // 使用 Repeater 获取第一项数据并绑定到标题
                Repeater {
                    model: PileModel
                    delegate: Item {
                        visible: false
                        height: 0
                        width: 0
                        Component.onCompleted: {
                            if (index === 0) {
                                headerDeviceId.text = model.deviceId || ""
                                headerDeviceType.text = model.deviceType || ""
                                headerLastCheckTime.text = model.lastCheckTime || ""
                            }
                        }
                    }
                }

                Label {
                    id: headerDeviceId
                    text: ""
                    font: AppFont.subtitle
                    color: AppTheme.textPrimary
                    Layout.fillWidth: true
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: AppLayout.spacingMedium

                    Label {
                        id: headerDeviceType
                        text: ""
                        font: AppFont.body
                        color: AppTheme.textSecondary
                    }

                    Label {
                        text: qsTr("检查时间：")
                        font: AppFont.body
                        color: AppTheme.textSecondary
                    }

                    Label {
                        id: headerLastCheckTime
                        text: ""
                        font: AppFont.body
                        color: AppTheme.textPrimary
                    }
                }
            }
        }

        ScrollView {
            anchors.top: headerArea.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            clip: true
            contentWidth: availableWidth

            ColumnLayout {
                id: listColumn
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: AppLayout.marginLarge
                spacing: AppLayout.spacingMedium

                Repeater {
                    model: PileModel

                    // 每个 CCU 一张可折叠卡片
                    Frame {
                        id: card
                        Layout.fillWidth: true
                        padding: 0

                        property bool expanded: true

                        // 粗略统计：异常项
                        readonly property int totalChecks: 20
                        readonly property int abnormalCount:
                            (model.ac1_stuck ? 1 : 0) +
                            (model.ac1_refuse ? 1 : 0) +
                            (model.ac2_stuck ? 1 : 0) +
                            (model.ac2_refuse ? 1 : 0) +
                            (model.par_pos_stuck ? 1 : 0) +
                            (model.par_pos_refuse ? 1 : 0) +
                            (model.par_neg_stuck ? 1 : 0) +
                            (model.par_neg_refuse ? 1 : 0) +
                            (model.fan1_stopped ? 1 : 0) +
                            (model.fan2_stopped ? 1 : 0) +
                            (model.fan3_stopped ? 1 : 0) +
                            (model.fan4_stopped ? 1 : 0) +
                            (model.gunA_pos_stuck ? 1 : 0) +
                            (model.gunA_pos_refuse ? 1 : 0) +
                            (model.gunA_neg_stuck ? 1 : 0) +
                            (model.gunA_neg_refuse ? 1 : 0) +
                            (model.gunB_pos_stuck ? 1 : 0) +
                            (model.gunB_pos_refuse ? 1 : 0) +
                            (model.gunB_neg_stuck ? 1 : 0) +
                            (model.gunB_neg_refuse ? 1 : 0)

                        readonly property int normalCount: totalChecks - abnormalCount

                        background: Rectangle {
                            color: AppTheme.backgroundPrimary
                            radius: AppLayout.radiusMedium
                            border.color: AppTheme.borderSubtle
                            border.width: 1
                        }

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: AppLayout.marginMedium
                            spacing: AppLayout.spacingSmall

                            // === 折叠头：标题 + 摘要 + 箭头 ===
                            RowLayout {
                                id: headerRow
                                Layout.fillWidth: true
                                Layout.preferredHeight: 40
                                spacing: AppLayout.spacingSmall

                                Label {
                                    text: qsTr("CCU：#%1").arg(model.ccuIndex)
                                    font: AppFont.subtitle
                                    color: AppTheme.foregroundPrimary
                                    Layout.fillWidth: true
                                }

                                Label {
                                    text: qsTr("正常：%1  异常：%2")
                                            .arg(card.normalCount)
                                            .arg(card.abnormalCount)
                                    font: AppFont.body
                                    color: card.abnormalCount > 0
                                           ? AppTheme.error
                                           : AppTheme.success
                                }

                                Label {
                                    font.family: AppFont.iconFontFamily
                                    font.pixelSize: AppFont.subtitleFontSize
                                    text: card.expanded ? "\uE5CE" : "\uE5CF" // expand_less / expand_more
                                    color: AppTheme.iconMuted
                                }

                                MouseArea {
                                    anchors.fill: undefined
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: card.expanded = !card.expanded
                                }
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                height: 1
                                color: AppTheme.borderSubtle
                                visible: card.expanded
                            }

                            // === 展开内容 ===
                            ColumnLayout {
                                id: detailColumn
                                spacing: AppLayout.spacingSmall
                                Layout.fillWidth: true
                                visible: card.expanded

                                // 交流接触器 1
                                GroupBox {
                                    title: qsTr("交流接触器 1")
                                    Layout.fillWidth: true

                                    Column {
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        anchors.margins: AppLayout.marginSmall
                                        spacing: 4

                                        StatusRow {
                                            label: qsTr("粘连")
                                            valueText: statusText(model.ac1_stuck)
                                            valueColor: statusColor(model.ac1_stuck)
                                        }
                                        StatusRow {
                                            label: qsTr("拒动")
                                            valueText: statusText(model.ac1_refuse)
                                            valueColor: statusColor(model.ac1_refuse)
                                        }
                                    }
                                }

                                // 交流接触器 2
                                GroupBox {
                                    title: qsTr("交流接触器 2")
                                    Layout.fillWidth: true

                                    Column {
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        anchors.margins: AppLayout.marginSmall
                                        spacing: 4

                                        StatusRow {
                                            label: qsTr("粘连")
                                            valueText: statusText(model.ac2_stuck)
                                            valueColor: statusColor(model.ac2_stuck)
                                        }
                                        StatusRow {
                                            label: qsTr("拒动")
                                            valueText: statusText(model.ac2_refuse)
                                            valueColor: statusColor(model.ac2_refuse)
                                        }
                                    }
                                }

                                // 并联接触器
                                GroupBox {
                                    title: qsTr("并联接触器")
                                    Layout.fillWidth: true

                                    Column {
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        anchors.margins: AppLayout.marginSmall
                                        spacing: 4

                                        StatusRow {
                                            label: qsTr("正极粘连")
                                            valueText: statusText(model.par_pos_stuck)
                                            valueColor: statusColor(model.par_pos_stuck)
                                        }
                                        StatusRow {
                                            label: qsTr("正极拒动")
                                            valueText: statusText(model.par_pos_refuse)
                                            valueColor: statusColor(model.par_pos_refuse)
                                        }
                                        StatusRow {
                                            label: qsTr("负极粘连")
                                            valueText: statusText(model.par_neg_stuck)
                                            valueColor: statusColor(model.par_neg_stuck)
                                        }
                                        StatusRow {
                                            label: qsTr("负极拒动")
                                            valueText: statusText(model.par_neg_refuse)
                                            valueColor: statusColor(model.par_neg_refuse)
                                        }
                                    }
                                }

                                // 风扇状态（这里也用 StatusRow 复用）
                                GroupBox {
                                    title: qsTr("风扇状态")
                                    Layout.fillWidth: true

                                    Column {
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        anchors.margins: AppLayout.marginSmall
                                        spacing: AppLayout.spacingSmall

                                        // Fan 1
                                        Column {
                                            width: parent.width
                                            Label { text: qsTr("风扇 1"); font: AppFont.bodyBold }

                                            StatusRow {
                                                label: qsTr("停转反馈")
                                                valueText: model.fan1_stopped ? qsTr("是") : qsTr("否")
                                                valueColor: model.fan1_stopped ? AppTheme.error : AppTheme.success
                                            }
                                            StatusRow {
                                                label: qsTr("转动反馈")
                                                valueText: model.fan1_rotating ? qsTr("是") : qsTr("否")
                                                valueColor: model.fan1_rotating ? AppTheme.success : AppTheme.muted
                                            }
                                        }

                                        Rectangle { width: parent.width; height: 1; color: AppTheme.borderSubtle }

                                        // Fan 2
                                        Column {
                                            width: parent.width
                                            Label { text: qsTr("风扇 2"); font: AppFont.bodyBold }

                                            StatusRow {
                                                label: qsTr("停转反馈")
                                                valueText: model.fan2_stopped ? qsTr("是") : qsTr("否")
                                                valueColor: model.fan2_stopped ? AppTheme.error : AppTheme.success
                                            }
                                            StatusRow {
                                                label: qsTr("转动反馈")
                                                valueText: model.fan2_rotating ? qsTr("是") : qsTr("否")
                                                valueColor: model.fan2_rotating ? AppTheme.success : AppTheme.muted
                                            }
                                        }

                                        Rectangle { width: parent.width; height: 1; color: AppTheme.borderSubtle }

                                        // Fan 3
                                        Column {
                                            width: parent.width
                                            Label { text: qsTr("风扇 3"); font: AppFont.bodyBold }

                                            StatusRow {
                                                label: qsTr("停转反馈")
                                                valueText: model.fan3_stopped ? qsTr("是") : qsTr("否")
                                                valueColor: model.fan3_stopped ? AppTheme.error : AppTheme.success
                                            }
                                            StatusRow {
                                                label: qsTr("转动反馈")
                                                valueText: model.fan3_rotating ? qsTr("是") : qsTr("否")
                                                valueColor: model.fan3_rotating ? AppTheme.success : AppTheme.muted
                                            }
                                        }

                                        Rectangle { width: parent.width; height: 1; color: AppTheme.borderSubtle }

                                        // Fan 4
                                        Column {
                                            width: parent.width
                                            Label { text: qsTr("风扇 4"); font: AppFont.bodyBold }

                                            StatusRow {
                                                label: qsTr("停转反馈")
                                                valueText: model.fan4_stopped ? qsTr("是") : qsTr("否")
                                                valueColor: model.fan4_stopped ? AppTheme.error : AppTheme.success
                                            }
                                            StatusRow {
                                                label: qsTr("转动反馈")
                                                valueText: model.fan4_rotating ? qsTr("是") : qsTr("否")
                                                valueColor: model.fan4_rotating ? AppTheme.success : AppTheme.muted
                                            }
                                        }
                                    }
                                }

                                // 枪 A 状态
                                GroupBox {
                                    title: qsTr("枪 A 状态")
                                    Layout.fillWidth: true

                                    Column {
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        anchors.margins: AppLayout.marginSmall
                                        spacing: 4

                                        StatusRow {
                                            label: qsTr("正极接触器粘连")
                                            valueText: statusText(model.gunA_pos_stuck)
                                            valueColor: statusColor(model.gunA_pos_stuck)
                                        }
                                        StatusRow {
                                            label: qsTr("正极接触器拒动")
                                            valueText: statusText(model.gunA_pos_refuse)
                                            valueColor: statusColor(model.gunA_pos_refuse)
                                        }
                                        StatusRow {
                                            label: qsTr("负极接触器粘连")
                                            valueText: statusText(model.gunA_neg_stuck)
                                            valueColor: statusColor(model.gunA_neg_stuck)
                                        }
                                        StatusRow {
                                            label: qsTr("负极接触器拒动")
                                            valueText: statusText(model.gunA_neg_refuse)
                                            valueColor: statusColor(model.gunA_neg_refuse)
                                        }
                                    }
                                }

                                // 枪 B 状态
                                GroupBox {
                                    title: qsTr("枪 B 状态")
                                    Layout.fillWidth: true

                                    Column {
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        anchors.margins: AppLayout.marginSmall
                                        spacing: 4

                                        StatusRow {
                                            label: qsTr("正极接触器粘连")
                                            valueText: statusText(model.gunB_pos_stuck)
                                            valueColor: statusColor(model.gunB_pos_stuck)
                                        }
                                        StatusRow {
                                            label: qsTr("正极接触器拒动")
                                            valueText: statusText(model.gunB_pos_refuse)
                                            valueColor: statusColor(model.gunB_pos_refuse)
                                        }
                                        StatusRow {
                                            label: qsTr("负极接触器粘连")
                                            valueText: statusText(model.gunB_neg_stuck)
                                            valueColor: statusColor(model.gunB_neg_stuck)
                                        }
                                        StatusRow {
                                            label: qsTr("负极接触器拒动")
                                            valueText: statusText(model.gunB_neg_refuse)
                                            valueColor: statusColor(model.gunB_neg_refuse)
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
