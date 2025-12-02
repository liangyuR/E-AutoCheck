// HomePage.qml
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15
import Qt5Compat.GraphicalEffects
import GUI

Page {
    id: page
    title: qsTr("设备总览")
    
    background: Rectangle {
        color: AppTheme.backgroundPrimary
    }

    // 后续可以改成来自 C++/Python 的 model
    property var deviceModel: [
        {
            deviceId: "CHG-001",
            name: "充电桩 A1",
            deviceType: "充电桩",
            ipAddress: "192.168.1.101",
            statusText: "正常",
            statusLevel: "normal",
            online: true,
            location: "一层西侧",
            lastCheck: "2025-12-02 10:15",
            checking: false,
            checkProgress: 0,
            checkTotal: 10
        },
        {
            deviceId: "CHG-002",
            name: "充电桩 A2",
            deviceType: "充电桩",
            ipAddress: "192.168.1.102",
            statusText: "通讯异常",
            statusLevel: "error",
            online: false,
            location: "一层西侧",
            lastCheck: "2025-12-01 22:30",
            checking: false,
            checkProgress: 0,
            checkTotal: 10
        },
        {
            deviceId: "CHG-003",
            name: "充电桩 A3",
            deviceType: "充电桩",
            ipAddress: "192.168.1.103",
            statusText: "自检中",
            statusLevel: "normal",
            online: true,
            location: "一层东侧",
            lastCheck: "2025-12-02 11:30",
            checking: true,
            checkProgress: 3,
            checkTotal: 10
        }
    ]

    // JavaScript 函数：按设备类型分组
    function groupDevicesByType() {
        var groups = {}
        for (var i = 0; i < deviceModel.length; i++) {
            var device = deviceModel[i]
            var type = device.deviceType
            if (!groups[type]) {
                groups[type] = []
            }
            groups[type].push(device)
        }
        return groups
    }

    ScrollView {
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth

        ColumnLayout {
            width: parent.width
            spacing: AppLayout.spacingLarge

            // 动态生成每个设备类型的分组
            Repeater {
                model: {
                    var groups = page.groupDevicesByType()
                    var typeList = []
                    for (var type in groups) {
                        typeList.push({
                            "deviceType": type,
                            "devices": groups[type]
                        })
                    }
                    return typeList
                }

                // 每个设备类型分组
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.topMargin: index === 0 ? AppLayout.marginLarge : 0
                    Layout.leftMargin: AppLayout.marginLarge
                    Layout.rightMargin: AppLayout.marginLarge
                    Layout.bottomMargin: AppLayout.marginMedium
                    spacing: AppLayout.spacingMedium

                    // 分组标题卡片
                    Rectangle {
                        Layout.fillWidth: true
                        height: 56
                        radius: AppLayout.radiusMedium
                        color: AppTheme.backgroundSecondary
                        border.width: 1
                        border.color: AppTheme.borderLight

                        layer.enabled: true
                        layer.effect: DropShadow {
                            horizontalOffset: 0
                            verticalOffset: 1
                            radius: 4
                            samples: 8
                            color: AppTheme.overlayLight
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: AppLayout.marginLarge
                            anchors.rightMargin: AppLayout.marginLarge
                            spacing: AppLayout.spacingMedium

                            Label {
                                text: modelData.deviceType
                                font: AppFont.title
                                color: AppTheme.textPrimary
                                Layout.fillWidth: true
                            }

                            Rectangle {
                                width: 36
                                height: 36
                                radius: 18
                                color: AppTheme.primary
                                opacity: 0.1

                                Label {
                                    anchors.centerIn: parent
                                    text: modelData.devices.length
                                    font: AppFont.subtitle
                                    color: AppTheme.primary
                                }
                            }
                        }
                    }

                    // 横向设备卡片列表
                    Flow {
                        Layout.fillWidth: true
                        spacing: AppLayout.spacingMedium

                        Repeater {
                            model: modelData.devices

                            Loader {
                                sourceComponent: modelData.deviceType === "充电桩" 
                                    ? chargingPileCardComponent 
                                    : deviceCardComponent

                                onLoaded: {
                                    item.deviceId = modelData.deviceId
                                    item.name = modelData.name
                                    item.statusText = modelData.statusText
                                    item.statusLevel = modelData.statusLevel
                                    item.online = modelData.online
                                    item.lastCheck = modelData.lastCheck
                                    item.checking = modelData.checking || false

                                    if (modelData.deviceType === "充电桩") {
                                        item.ipAddress = modelData.ipAddress || ""
                                        item.checkProgress = modelData.checkProgress || 0
                                        item.checkTotal = modelData.checkTotal || 10
                                    } else {
                                        item.deviceType = modelData.deviceType
                                        item.location = modelData.location
                                    }

                                    item.selfCheckRequested.connect(function(deviceId) {
                                        if (typeof SelfCheck !== "undefined") {
                                            SelfCheck.triggerSelfCheck(deviceId)
                                        }
                                        modelData.checking = true
                                    })

                                    // 连接长按信号（仅充电桩）
                                    if (item.cardLongPressed) {
                                        item.cardLongPressed.connect(function(deviceId) {
                                            console.log("Long pressed charging pile:", deviceId)
                                            // TODO: 导航到详情页面
                                            // stackView.push(detailPage, {deviceId: deviceId})
                                        })
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // 充电桩卡片组件
    Component {
        id: chargingPileCardComponent
        ChargingPileCard {}
    }

    // 通用设备卡片组件
    Component {
        id: deviceCardComponent
        DeviceCard {}
    }
}
