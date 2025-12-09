import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import GUI

RowLayout {
  id: root
  // 语义属性
  property string label
  property string valueText
  property color valueColor

  Layout.fillWidth: true
  spacing: AppLayout.spacingSmall

  Label {
    text: label
    Layout.fillWidth: true
    font: AppFont.body
    color: AppTheme.foregroundPrimary
    wrapMode: Text.Wrap
  }

  Label {
    text: valueText
    font: AppFont.body
    color: valueColor
    wrapMode: Text.Wrap
  }
}
