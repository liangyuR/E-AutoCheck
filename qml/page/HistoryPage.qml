import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls.Material 2.15
import GUI
import EAutoCheck 1.0

Page {
    id: page
    title: qsTr("自检历史")

    property string deviceId: ""
    signal backRequested()
    signal recordRequested(string recordId)

    Component.onCompleted: {
        if (deviceId.length > 0) {
            HistoryModel.load(deviceId, 20)
        }
    }

    onDeviceIdChanged: {
        if (deviceId.length > 0) {
            HistoryModel.load(deviceId, 20)
        }
    }

    background: Rectangle {
        color: AppTheme.backgroundPrimary
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: AppLayout.marginLarge
        spacing: AppLayout.spacingLarge

        // Header actions
        RowLayout {
            Layout.fillWidth: true
            spacing: AppLayout.spacingMedium

            ToolButton {
                id: backButton
                Layout.preferredWidth: AppLayout.touchButtonHeight
                Layout.preferredHeight: AppLayout.touchButtonHeight
                font.family: AppFont.iconFontFamily
                font.pixelSize: AppFont.subtitleFontSize
                text: "\uE5C4"
                Material.foreground: AppTheme.foregroundPrimary
                background: null
                onClicked: page.backRequested()
            }

            Label {
                Layout.fillWidth: true
                text: qsTr("设备：%1").arg(deviceId.length > 0 ? deviceId : qsTr("未知设备"))
                color: AppTheme.foregroundPrimary
                font: AppFont.title
                elide: Text.ElideRight
            }

            Button {
                text: qsTr("刷新")
                onClicked: {
                    if (deviceId.length > 0) {
                        HistoryModel.load(deviceId, 20)
                    }
                }
            }
        }

        // List content
        Frame {
            Layout.fillWidth: true
            Layout.fillHeight: true
            padding: 0
            background: Rectangle { color: AppTheme.backgroundSecondary; radius: 8 }

            ListView {
                id: historyList
                anchors.fill: parent
                clip: true
                model: HistoryModel
                delegate: Item {
                    width: historyList.width
                    height: contentRow.implicitHeight + AppLayout.spacingMedium * 2

                    Rectangle {
                        anchors.fill: parent
                        anchors.margins: AppLayout.spacingSmall
                        radius: 8
                        color: AppTheme.backgroundPrimary
                        border.color: AppTheme.borderSubtle
                        border.width: 1
                    }

                    ColumnLayout {
                        id: contentRow
                        anchors.fill: parent
                        anchors.margins: AppLayout.spacingMedium
                        spacing: AppLayout.spacingSmall

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: AppLayout.spacingSmall

                            Label {
                                Layout.fillWidth: true
                                text: timestampDisplay
                                color: AppTheme.foregroundSecondary
                                font.pixelSize: AppFont.subtitleFontSize
                            }

                            Rectangle {
                                radius: 10
                                height: 24
                                width: Math.max(statusLabel.implicitWidth + 16, 64)
                                color: statusColor(status)

                                Label {
                                    id: statusLabel
                                    anchors.centerIn: parent
                                    text: status
                                    color: AppTheme.textOnPrimary
                                    font.pixelSize: AppFont.captionFontSize
                                }
                            }
                        }

                        Label {
                            Layout.fillWidth: true
                            text: summary.length > 0 ? summary : qsTr("无摘要")
                            color: AppTheme.foregroundPrimary
                            wrapMode: Text.WordWrap
                        }

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("记录ID: %1").arg(recordId)
                            color: AppTheme.foregroundTertiary
                            font.pixelSize: AppFont.captionFontSize
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: page.recordRequested(recordId)
                    }
                }

                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                footer: Label {
                    width: historyList.width
                    horizontalAlignment: Text.AlignHCenter
                    padding: AppLayout.spacingMedium
                            text: historyList.count === 0 ? qsTr("暂无历史记录") : qsTr("已加载全部记录")
                    color: AppTheme.foregroundSecondary
                }
            }
        }
    }

    function statusColor(status) {
        switch (status) {
        case "normal":
            return AppTheme.success;
        case "warning":
            return AppTheme.warning;
        case "error":
            return AppTheme.error;
        default:
            return AppTheme.foregroundTertiary;
        }
    }
}

