// HomePage.qml
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15
import Qt5Compat.GraphicalEffects
import GUI
import EAutoCheck 1.0 

Page {
    id: page
    title: qsTr("设备总览")
    
    background: Rectangle {
        color: AppTheme.backgroundPrimary
    }

    ScrollView {
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth

        ColumnLayout {
            width: parent.width
            spacing: AppLayout.spacingLarge

            // 顶部统计或标题区域（可选，这里先留白或保留原来的逻辑）
            
            // 设备列表区域
            Flow {
                Layout.fillWidth: true
                Layout.margins: AppLayout.marginLarge
                spacing: AppLayout.spacingMedium

                Repeater {
                    model: DeviceManager

                    Loader {
                        id: cardLoader
                        // 根据 type 选择组件
                        // 注意：C++ 返回的 type 可能是 "PILE", "STACK" 等
                        property bool isPile: model.type === "PILE" || model.type === "STACK"
                        
                        sourceComponent: isPile 
                            ? chargingPileCardComponent 
                            : deviceCardComponent

                        onLoaded: {
                            // 一次性设置不需要动态更新的属性
                            item.deviceId = model.equipNo
                            item.name = model.name
                            if (isPile) {
                                item.ipAddress = model.ipAddr
                                // item.checkProgress = ... // 需要 C++ 提供
                                // item.checkTotal = ...
                            } else {
                                item.location = model.stationNo // 暂用 stationNo 代替 location
                            }
                        }

                        // 使用 Binding 元素绑定需要动态更新的属性
                        Binding {
                            target: cardLoader.item
                            property: "online"
                            value: model.isOnline
                            when: cardLoader.item !== null
                        }

                        Binding {
                            target: cardLoader.item
                            property: "statusText"
                            // 使用 DeviceManager 提供的状态文本，如果没有则使用默认值
                            value: model.statusText || (model.isOnline ? "正常" : "离线")
                            when: cardLoader.item !== null
                        }

                        Binding {
                            target: cardLoader.item
                            property: "statusLevel"
                            value: model.isOnline ? "normal" : "error"
                            when: cardLoader.item !== null
                        }

                        Binding {
                            target: cardLoader.item
                            property: "checking"
                            value: model.isChecking || false
                            when: cardLoader.item !== null
                        }

                        Binding {
                            target: cardLoader.item
                            property: "lastCheck"
                            value: model.lastCheckTime || ""
                            when: cardLoader.item !== null
                        }

                        Connections {
                            target: cardLoader.item
                            function onSelfCheckRequested(id) {
                                // 调用 C++ 接口
                                if (typeof CheckManager !== "undefined") {
                                     CheckManager.CheckDevice(id)
                                } else {
                                     console.warn("CheckManager not registered!")
                                }
                            }
                            
                            function onCardLongPressed(id) {
                                console.log("Long pressed:", id)
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
