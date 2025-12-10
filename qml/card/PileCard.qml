// PileCard.qml - 充电桩卡片（继承 DeviceCard）
import QtQuick 2.15
import GUI

DeviceCard {
    id: root

    // 充电桩专用图标
    deviceIcon: "\uE1E0"  // Material Icons: ev_station

    // 充电桩专用配色（绿色渐变）
    headerColorStart: Qt.lighter(AppTheme.primary, 1.3)
    headerColorEnd: AppTheme.primary
}
