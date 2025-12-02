// ChargingPileCard.qml
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15
import Qt5Compat.GraphicalEffects
import GUI

Control {
    id: root

    // 充电桩属性
    property string deviceId: ""
    property string name: ""
    property string ipAddress: ""
    property string lastCheck: ""
    property bool online: true
    property string statusText: ""
    property string statusLevel: "normal" // "normal" / "warning" / "error"
    property bool checking: false
    property int checkProgress: 0      // 当前进度
    property int checkTotal: 10        // 总项数

    // 长按进度
    property real longPressProgress: 0.0

    // 信号
    signal selfCheckRequested(string deviceId)
    signal cardLongPressed(string deviceId)

    implicitWidth: 340
    implicitHeight: 240
    padding: 0

    // 长按检测
    MouseArea {
        id: longPressArea
        anchors.fill: parent
        propagateComposedEvents: true
        
        onPressAndHold: {
            if (longPressTimer.running) {
                longPressTimer.stop()
                root.longPressProgress = 0
            }
            root.cardLongPressed(root.deviceId)
        }
        
        onPressed: {
            mouse.accepted = false
            longPressTimer.start()
        }
        
        onReleased: {
            longPressTimer.stop()
            progressResetAnimation.start()
        }
        
        onCanceled: {
            longPressTimer.stop()
            progressResetAnimation.start()
        }
    }

    // 长按进度计时器
    Timer {
        id: longPressTimer
        interval: 16  // 约60fps
        repeat: true
        property int elapsedTime: 0
        property int totalDuration: 800  // 800ms 完成长按
        
        onTriggered: {
            elapsedTime += interval
            root.longPressProgress = Math.min(1.0, elapsedTime / totalDuration)
        }
        
        onRunningChanged: {
            if (running) {
                elapsedTime = 0
                root.longPressProgress = 0
            }
        }
    }

    // 进度重置动画
    NumberAnimation {
        id: progressResetAnimation
        target: root
        property: "longPressProgress"
        to: 0
        duration: 200
        easing.type: Easing.OutQuad
    }

    background: Rectangle {
        id: cardBackground
        radius: AppLayout.radiusLarge
        color: AppTheme.backgroundSecondary
        border.width: root.statusLevel === "error" ? 2 : 1
        border.color: root.statusLevel === "error" ? AppTheme.error
                        : root.statusLevel === "warning" ? AppTheme.warning
                        : AppTheme.borderLight

        layer.enabled: true
        layer.effect: DropShadow {
            horizontalOffset: 0
            verticalOffset: 4
            radius: 12
            samples: 24
            color: Qt.rgba(0, 0, 0, 0.08)
        }

        // 长按进度边框效果
        Rectangle {
            id: progressBorder
            anchors.fill: parent
            radius: parent.radius
            color: "transparent"
            border.width: 3
            border.color: "transparent"
            visible: root.longPressProgress > 0

            // 使用ConicalGradient创建圆形进度效果
            layer.enabled: true
            layer.effect: OpacityMask {
                maskSource: Rectangle {
                    width: progressBorder.width
                    height: progressBorder.height
                    radius: progressBorder.radius
                    color: "transparent"
                    border.width: progressBorder.border.width
                    border.color: "white"
                }
            }

            Rectangle {
                id: progressOverlay
                anchors.fill: parent
                radius: parent.radius
                color: "transparent"
                
                // 创建渐变进度效果
                ConicalGradient {
                    anchors.fill: parent
                    angle: -90  // 从顶部开始
                    
                    gradient: Gradient {
                        GradientStop { 
                            position: 0.0
                            color: Qt.rgba(
                                AppTheme.primary.r,
                                AppTheme.primary.g,
                                AppTheme.primary.b,
                                0.9
                            )
                        }
                        GradientStop { 
                            position: root.longPressProgress
                            color: Qt.rgba(
                                AppTheme.primary.r,
                                AppTheme.primary.g,
                                AppTheme.primary.b,
                                0.9
                            )
                        }
                        GradientStop { 
                            position: root.longPressProgress + 0.01
                            color: "transparent"
                        }
                        GradientStop { 
                            position: 1.0
                            color: "transparent"
                        }
                    }
                }
            }
        }

        // 边框遮罩，只显示边缘部分
        Rectangle {
            anchors.fill: parent
            anchors.margins: 3
            radius: parent.radius - 3
            color: AppTheme.backgroundSecondary
            visible: root.longPressProgress > 0
        }
    }

    contentItem: ColumnLayout {
        spacing: 0

        // 顶部渐变色区域 - 设备名称 + 状态
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 72
            radius: AppLayout.radiusLarge
            clip: true

            gradient: Gradient {
                GradientStop {
                    position: 0.0
                    color: root.online ? Qt.lighter(AppTheme.primary, 1.3) : "#BDBDBD"
                }
                GradientStop {
                    position: 1.0
                    color: root.online ? AppTheme.primary : "#9E9E9E"
                }
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: AppLayout.marginLarge
                anchors.rightMargin: AppLayout.marginLarge
                spacing: AppLayout.spacingMedium

                // 设备图标
                Rectangle {
                    width: 48
                    height: 48
                    radius: 24
                    color: Qt.rgba(255, 255, 255, 0.3)
                    Layout.alignment: Qt.AlignVCenter

                    Label {
                        anchors.centerIn: parent
                        text: "\uE1E0" // Material Icons: ev_station
                        font.family: AppFont.iconFontFamily
                        font.pixelSize: 28
                        color: "white"
                    }
                }

                // 设备名称
                Label {
                    text: root.name
                    font: Qt.font({
                        family: AppFont.fontFamily,
                        pixelSize: 20,
                        weight: Font.Medium
                    })
                    color: "white"
                    Layout.fillWidth: true
                }

                // 在线状态徽章
                Rectangle {
                    width: 68
                    height: 28
                    radius: 14
                    color: Qt.rgba(255, 255, 255, 0.25)
                    Layout.alignment: Qt.AlignVCenter

                    RowLayout {
                        anchors.centerIn: parent
                        spacing: 4

                        Rectangle {
                            width: 8
                            height: 8
                            radius: 4
                            color: root.online ? "#4CAF50" : "#F44336"
                        }

                        Label {
                            text: root.online ? "在线" : "离线"
                            font: AppFont.caption
                            color: "white"
                        }
                    }
                }
            }
        }

        // 中部信息区域
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: AppLayout.marginLarge
            spacing: AppLayout.spacingMedium

            // 设备信息网格
            GridLayout {
                Layout.fillWidth: true
                columns: 2
                rowSpacing: AppLayout.spacingMedium
                columnSpacing: AppLayout.spacingLarge

                // 设备ID
                RowLayout {
                    Layout.fillWidth: true
                    spacing: AppLayout.spacingSmall

                    Label {
                        text: "\uE30A" // Material Icons: label
                        font.family: AppFont.iconFontFamily
                        font.pixelSize: 16
                        color: AppTheme.textSecondary
                    }

                    Label {
                        text: "ID: " + root.deviceId
                        font: AppFont.body
                        color: AppTheme.textPrimary
                    }
                }

                // IP地址
                RowLayout {
                    Layout.fillWidth: true
                    spacing: AppLayout.spacingSmall

                    Label {
                        text: "\uE30B" // Material Icons: dns
                        font.family: AppFont.iconFontFamily
                        font.pixelSize: 16
                        color: AppTheme.textSecondary
                    }

                    Label {
                        text: root.ipAddress || "N/A"
                        font: AppFont.body
                        color: AppTheme.textPrimary
                    }
                }

                // 最后检测时间
                RowLayout {
                    Layout.fillWidth: true
                    Layout.columnSpan: 2
                    spacing: AppLayout.spacingSmall

                    Label {
                        text: "\uE8B5" // Material Icons: schedule
                        font.family: AppFont.iconFontFamily
                        font.pixelSize: 16
                        color: AppTheme.textSecondary
                    }

                    Label {
                        text: "最后检测: " + (root.lastCheck || "从未")
                        font: AppFont.caption
                        color: AppTheme.textSecondary
                    }
                }
            }

            // 状态指示器
            Rectangle {
                Layout.fillWidth: true
                height: 32
                radius: AppLayout.radiusSmall
                color: root.statusLevel === "error" ? Qt.rgba(244, 67, 54, 0.1)
                       : root.statusLevel === "warning" ? Qt.rgba(255, 152, 0, 0.1)
                       : Qt.rgba(76, 175, 80, 0.1)

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: AppLayout.marginMedium
                    anchors.rightMargin: AppLayout.marginMedium
                    spacing: AppLayout.spacingSmall

                    Rectangle {
                        width: 10
                        height: 10
                        radius: 5
                        color: root.statusLevel === "error" ? AppTheme.error
                               : root.statusLevel === "warning" ? AppTheme.warning
                               : AppTheme.success
                    }

                    Label {
                        text: root.statusText
                        font: AppFont.body
                        color: root.statusLevel === "error" ? AppTheme.error
                               : root.statusLevel === "warning" ? AppTheme.warning
                               : AppTheme.success
                        Layout.fillWidth: true
                    }
                }
            }

            Item {
                Layout.fillHeight: true
            }
        }

        // 底部操作区域
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            color: AppTheme.backgroundTertiary
            radius: AppLayout.radiusLarge

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: AppLayout.marginLarge
                anchors.rightMargin: AppLayout.marginLarge
                spacing: AppLayout.spacingMedium

                // 进度显示（仅在自检时显示）
                RowLayout {
                    visible: root.checking
                    spacing: AppLayout.spacingMedium
                    Layout.fillWidth: true

                    // 圆形进度指示器
                    Rectangle {
                        width: 36
                        height: 36
                        radius: 18
                        color: "transparent"
                        border.width: 3
                        border.color: AppTheme.borderLight

                        Rectangle {
                            width: 30
                            height: 30
                            radius: 15
                            anchors.centerIn: parent
                            color: AppTheme.primary
                            opacity: 0.2
                        }

                        BusyIndicator {
                            anchors.centerIn: parent
                            width: 32
                            height: 32
                            running: root.checking
                        }
                    }

                    ColumnLayout {
                        spacing: 2

                        Label {
                            text: "自检中..."
                            font: Qt.font({
                                family: AppFont.fontFamily,
                                pixelSize: AppFont.bodyFontSize,
                                weight: Font.Medium
                            })
                            color: AppTheme.textPrimary
                        }

                        Label {
                            text: root.checkProgress + " / " + root.checkTotal + " 项"
                            font: AppFont.caption
                            color: AppTheme.textSecondary
                        }
                    }

                    // 进度条
                    ProgressBar {
                        Layout.fillWidth: true
                        from: 0
                        to: root.checkTotal
                        value: root.checkProgress
                        Material.accent: AppTheme.primary
                    }
                }

                // 提示信息（非自检时显示）
                Label {
                    visible: !root.checking
                    text: "长按查看详情"
                    font: AppFont.caption
                    color: AppTheme.textTertiary
                    Layout.fillWidth: true
                }

                // 自检按钮
                Button {
                    text: root.checking ? "停止" : "自检"
                    enabled: root.online
                    Layout.preferredWidth: 100
                    Material.elevation: root.checking ? 0 : 4
                    highlighted: !root.checking
                    flat: root.checking
                    onClicked: root.selfCheckRequested(root.deviceId)
                }
            }
        }
    }
}

