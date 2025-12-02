pragma Singleton
import QtQuick
import QtQuick.Controls.Material

QtObject {
    // Background colors
    readonly property color backgroundPrimary: "#EEF4F9"
    readonly property color backgroundSecondary: "#FFFFFF"
    readonly property color backgroundTertiary: "#F5F5F5"

    // Text colors
    readonly property color textPrimary: "#000000"
    readonly property color textSecondary: "#666666"
    readonly property color textTertiary: "#999999"
    readonly property color textInverse: "#FFFFFF"

    // Accent colors
    readonly property color accent: "#1565C0" // Match qtquickcontrols2.conf Accent
    readonly property color primary: "#2196F3" // Match qtquickcontrols2.conf Primary (Blue)

    // State colors
    readonly property color success: "#4CAF50"
    readonly property color warning: "#FF9800"
    readonly property color error: "#F44336"
    readonly property color info: "#2196F3"

    // Border colors
    readonly property color borderLight: "#E0E0E0"
    readonly property color borderMedium: "#BDBDBD"
    readonly property color borderDark: "#757575"

    // Overlay colors
    readonly property color overlayLight: Qt.rgba(0, 0, 0, 0.1)
    readonly property color overlayMedium: Qt.rgba(0, 0, 0, 0.3)
    readonly property color overlayDark: Qt.rgba(0, 0, 0, 0.5)
}

