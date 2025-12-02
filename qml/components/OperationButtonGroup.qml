import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

// 通用操作按钮组组件
Item {
    id: operationButtonGroup

    property var operations: [] // 操作数组，格式: [{text: "按钮文本", action: "操作参数"}]

    property string title: "操作"

    signal operationClicked(string action)

    ColumnLayout {
        anchors.fill: parent
        spacing: layoutStyle.spacingMedium

        // 标题
        Label {
            font: fontStyle.subtitle
            text: title
        }

        // 横向排列操作按钮
        RowLayout {
            Layout.fillWidth: true
            Material.foreground: "white"
            spacing: layoutStyle.spacingMedium

            Repeater {
                model: operations

                Button {
                    Material.background: modelData.action.indexOf('c') !== -1 ? Material.primary : Material.Grey
                    font: fontStyle.body
                    text: modelData.text

                    onClicked: {
                        operationClicked(modelData.action);
                    }
                }
            }
        }
    }
}
