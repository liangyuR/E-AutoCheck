import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls.Material 2.15
import GUI
import EAutoCheck 1.0

Item {
    id: root
    anchors.fill: parent

    // 导航信号
    signal backRequested()
    signal historyRequested(string deviceId)

    // 详细组件（当前状态用）
    property Component detailComponent

    property string deviceId: ""

    // 当前选中的历史索引
    property int historyIndex: 0

    onDeviceIdChanged: {
        if (deviceId.length === 16) {
            HistoryModel.load(deviceId, 10)
        } else {
            console.warn("无效的设备ID长度:", deviceId)
        }
    }

    // =========================
    // 顶部标题栏：固定在页面最上方
    // =========================
    ToolBar {
        id: headerBar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 56
        z: 10                      // 确保在内容区上面

        background: null

        RowLayout {
            anchors.fill: parent
            anchors.margins: AppLayout.marginMedium
            spacing: AppLayout.spacingMedium

            // 返回按钮
            ToolButton {
                id: backButton
                Layout.preferredWidth: AppLayout.touchButtonHeight
                Layout.preferredHeight: AppLayout.touchButtonHeight

                font.family: AppFont.iconFontFamily
                font.pixelSize: AppFont.subtitleFontSize
                text: "\uE5C4"
                Material.foreground: AppTheme.iconOnHeader

                background: null
                onClicked: root.backRequested()
            }

            // 标题：设备名称 + 设备ID
            Label {
                id: titleLabel
                Layout.fillWidth: true

                text: qsTr("设备名称：%1  设备ID：%2")
                          .arg(deviceName.length > 0 ? deviceName : qsTr("未知"))
                          .arg(deviceId.length > 0 ? deviceId : qsTr("未知"))

                elide: Text.ElideRight
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
                font: AppFont.title
                color: AppTheme.foregroundPrimary
            }

            Button {
                id: historyButton
                text: qsTr("查看历史")
                onClicked: {
                    if (deviceId.length === 0) {
                        console.warn("缺少设备ID，无法查看历史")
                        return
                    }
                    root.historyRequested(deviceId)
                }
            }
        }
    }

    // =========================
    // 内容区域：明确从 headerBar 下面开始
    // =========================
    Rectangle {
        id: contentArea
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: headerBar.bottom     // 关键：顶边从 header 的底边开始
        anchors.bottom: parent.bottom
        color: AppTheme.backgroundPrimary
        z: 0

        Loader {
            id: contentLoader
            anchors.fill: parent
            anchors.margins: AppLayout.marginLarge

            sourceComponent: root.detailComponent

            onStatusChanged: {
                if (status === Loader.Error) {
                    console.warn("SelfCheckDetailPage: contentLoader error:", errorString())
                }
            }
        }
    }
}
