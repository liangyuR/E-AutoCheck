import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GUI
import EAutoCheck 1.0

ApplicationWindow {
    id: window

    height: 1400
    width: 2240
    title: "E-AutoCheck"
    visible: true

    // Load icon font
    FontLoader {
        id: iconFontLoader
        source: "qrc:/qt/qml/GUI/assets/fonts/MaterialIcons-Regular.ttf"
        onStatusChanged: {
            if (status === FontLoader.Error) {
                console.warn("Failed to load icon font. Please ensure MaterialIcons-Regular.ttf is in assets/fonts/")
            }
        }
    }

    background: Rectangle {
        color: AppTheme.backgroundPrimary
    }

    // 主布局
    ColumnLayout {
        anchors.fill: parent

        // 主要内容区域
        RowLayout {
            Layout.fillHeight: true
            Layout.fillWidth: true

            // 左侧导航栏
            Pane {
                Layout.fillHeight: true
                Layout.preferredWidth: AppLayout.navigationBarWidth

                background: Rectangle {
                    color: AppTheme.backgroundPrimary
                }

                ColumnLayout {
                    anchors.fill: parent
                    spacing: AppLayout.spacingLarge

                    Label {
                        Layout.alignment: Qt.AlignHCenter
                        Layout.fillWidth: true
                        Layout.topMargin: AppLayout.marginMedium
                        font: AppFont.title
                        text: "E-AutoCheck" + "\n" + "0.0.1"
                    }
                    ListView {
                        id: navListView

                        Layout.fillHeight: true
                        Layout.fillWidth: true

                        delegate: ItemDelegate {
                            height: AppLayout.touchButtonHeight
                            width: parent.width

                            background: Rectangle {
                                color: navListView.currentIndex === index ? AppTheme.accent : "transparent"
                                opacity: navListView.currentIndex === index ? 0.1 : 0
                            }

                            onClicked: {
                                navListView.currentIndex = index;
                                switch (model.page) {
                                case "homePage":
                                    mainStackView.replace(homePage);
                                    break;
                                case "settingsPage":
                                    mainStackView.replace(settingsPage);
                                    break;
                                }
                            }

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: AppLayout.marginMedium
                                anchors.rightMargin: AppLayout.marginMedium
                                spacing: AppLayout.spacingMedium

                                Label {
                                    font.family: AppFont.iconFontFamily
                                    font.pixelSize: AppFont.subtitleFontSize
                                    text: model.icon
                                }
                                Label {
                                    Layout.fillWidth: true
                                    font: AppFont.body
                                    text: model.name
                                }
                                Rectangle {
                                    color: AppTheme.accent
                                    height: parent.height - AppLayout.marginMedium
                                    radius: AppLayout.radiusSmall
                                    visible: navListView.currentIndex === index
                                    width: 4
                                }
                            }
                        }
                        model: ListModel {
                            ListElement {
                                icon: "\uE88A"
                                name: "Home"
                                page: "homePage"
                            }
                            ListElement {
                                icon: "\uE8B8"
                                name: "Settings"
                                page: "settingsPage"
                            }
                        }
                    }
                }
            }

            // 右侧内容区域
            StackView {
                id: mainStackView

                Layout.fillHeight: true
                Layout.fillWidth: true
                initialItem: homePage

                // 页面切换动画
                replaceEnter: Transition {
                    PropertyAnimation {
                        duration: AppLayout.touchAnimationDuration
                        from: 0
                        property: "opacity"
                        to: 1
                    }
                }
                replaceExit: Transition {
                    PropertyAnimation {
                        duration: AppLayout.touchAnimationDuration
                        from: 1
                        property: "opacity"
                        to: 0
                    }
                }
            }
        }
    }

    Component {
        id: homePage
        Home {
          id: homeRoot
          Connections {
            target: homeRoot
            function onToItemDetailPageRequested(deviceId) {
              console.log("触发切换到详细页面，当前设备ID：", deviceId)
              var page = mainStackView.replace(detialPage, {
                detailComponent: ccuComponent
              })
              if (page && page.loadData) {
                page.loadData(deviceId)
              } else {
                console.warn("未能加载详情页，跳过数据初始化")
              }
            }
          }
        }
    }

    Component {
        id: detialPage
        Detial {
            id: detialRoot
            Connections {
                target: detialRoot
                function onBackRequested() {
                    console.log("触发返回主页")
                    mainStackView.replace(homePage)
                }
                function onHistoryRequested(deviceId) {
                    console.log("进入历史页面，设备ID：", deviceId)
                    mainStackView.replace(historyPage, { deviceId: deviceId })
                }
            }
        }
    }

    Component {
        id: ccuComponent
        PileDetail {
        }
    }

    Component {
        id: historyPage
        HistoryPage {
            onBackRequested: {
                mainStackView.pop()
            }
            onRecordRequested: function(recordId) {
                PileModel.loadFromHistory(recordId);
                // 从历史记录跳转到对应设备的详情页，携带 recordId 便于后续扩展
                mainStackView.replace(detialPage, {
                    deviceId: deviceId,
                    detailComponent: ccuComponent
                })
            }
        }
    }

    Component {
        id: settingsPage

        ScrollView {
            clip: true
        }
    }
}
