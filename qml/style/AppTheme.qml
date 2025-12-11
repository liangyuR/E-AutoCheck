pragma Singleton
import QtQuick

QtObject {
  property bool darkMode: false

  // ==================== Background Colors ====================
  readonly property color backgroundPrimary: darkMode ? "#1E1E1E" : "#EEF4F9"
  readonly property color backgroundSecondary: darkMode ? "#252526" : "#FFFFFF"
  readonly property color backgroundTertiary: darkMode ? "#2D2D30" : "#F5F5F5"
  readonly property color backgroundElevated: darkMode ? "#333337" : "#FFFFFF"

  // ==================== Foreground Colors ====================
  readonly property color foregroundPrimary: darkMode ? "#E0E0E0" : "#212121"
  readonly property color foregroundSecondary: darkMode ? "#A0A0A0" : "#666666"
  readonly property color foregroundMuted: darkMode ? "#6E6E6E" : "#9E9E9E"

  // ==================== Text Colors ====================
  readonly property color textPrimary: darkMode ? "#FFFFFF" : "#000000"
  readonly property color textSecondary: darkMode ? "#B0B0B0" : "#666666"
  readonly property color textTertiary: darkMode ? "#808080" : "#999999"
  readonly property color textInverse: darkMode ? "#000000" : "#FFFFFF"
  readonly property color textOnPrimary: "#FFFFFF"
  readonly property color textDisabled: darkMode ? "#505050" : "#BDBDBD"

  // ==================== Accent / Primary Colors ====================
  readonly property color accent: darkMode ? "#4FC3F7" : "#1565C0"
  readonly property color primary: darkMode ? "#64B5F6" : "#2196F3"
  readonly property color primaryLight: darkMode ? "#90CAF9" : "#BBDEFB"
  readonly property color primaryDark: darkMode ? "#1976D2" : "#0D47A1"

  // ==================== State Colors ====================
  readonly property color success: darkMode ? "#81C784" : "#4CAF50"
  readonly property color successLight: darkMode ? "#A5D6A7" : "#C8E6C9"
  readonly property color warning: darkMode ? "#FFB74D" : "#FF9800"
  readonly property color warningLight: darkMode ? "#FFE082" : "#FFECB3"
  readonly property color error: darkMode ? "#E57373" : "#F44336"
  readonly property color errorLight: darkMode ? "#EF9A9A" : "#FFCDD2"
  readonly property color info: darkMode ? "#4FC3F7" : "#2196F3"
  readonly property color infoLight: darkMode ? "#81D4FA" : "#BBDEFB"
  readonly property color muted: darkMode ? "#616161" : "#9E9E9E"

  // ==================== Border Colors ====================
  readonly property color borderLight: darkMode ? "#3C3C3C" : "#E0E0E0"
  readonly property color borderMedium: darkMode ? "#4A4A4A" : "#BDBDBD"
  readonly property color borderDark: darkMode ? "#5A5A5A" : "#757575"
  readonly property color borderSubtle: darkMode ? "#333333" : "#EEEEEE"
  readonly property color borderFocus: darkMode ? "#4FC3F7" : "#1565C0"

  // ==================== Icon Colors ====================
  readonly property color iconPrimary: darkMode ? "#E0E0E0" : "#424242"
  readonly property color iconSecondary: darkMode ? "#9E9E9E" : "#757575"
  readonly property color iconMuted: darkMode ? "#616161" : "#9E9E9E"
  readonly property color iconOnHeader: darkMode ? "#FFFFFF" : "#000000"
  readonly property color iconActive: darkMode ? "#4FC3F7" : "#1565C0"

  // ==================== Surface Colors ====================
  readonly property color surfaceCard: darkMode ? "#2D2D30" : "#FFFFFF"
  readonly property color surfaceHover: darkMode ? "#3C3C3C" : "#F5F5F5"
  readonly property color surfacePressed: darkMode ? "#4A4A4A" : "#EEEEEE"
  readonly property color surfaceSelected: darkMode ? "#264F78" : "#E3F2FD"

  // ==================== Overlay Colors ====================
  readonly property color overlayLight: darkMode ? Qt.rgba(1, 1, 1, 0.05) : Qt.rgba(0, 0, 0, 0.1)
  readonly property color overlayMedium: darkMode ? Qt.rgba(1, 1, 1, 0.1) : Qt.rgba(0, 0, 0, 0.3)
  readonly property color overlayDark: darkMode ? Qt.rgba(1, 1, 1, 0.15) : Qt.rgba(0, 0, 0, 0.5)
  readonly property color overlayScrim: darkMode ? Qt.rgba(0, 0, 0, 0.7) : Qt.rgba(0, 0, 0, 0.5)

  // ==================== Shadow Colors ====================
  readonly property color shadowLight: darkMode ? Qt.rgba(0, 0, 0, 0.3) : Qt.rgba(0, 0, 0, 0.1)
  readonly property color shadowMedium: darkMode ? Qt.rgba(0, 0, 0, 0.5) : Qt.rgba(0, 0, 0, 0.2)

  // ==================== Divider ====================
  readonly property color divider: darkMode ? "#3C3C3C" : "#E0E0E0"

  // ==================== Helper Function ====================
  function toggleTheme() {
    darkMode = !darkMode
  }
}
