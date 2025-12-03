import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GUI

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
                                    stackView.replace(homePage);
                                    break;
                                case "settingsPage":
                                    stackView.replace(settingsPage);
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
                id: stackView

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
        }
    }

    Component {
        id: settingsPage

        ScrollView {
            clip: true
        }
    }
}
