pragma Singleton
import QtQuick

QtObject {
  // ==================== Spacing ====================
  readonly property int spacingTiny: 4
  readonly property int spacingSmall: 8
  readonly property int spacingMedium: 16
  readonly property int spacingLarge: 24
  readonly property int spacingXLarge: 32
  readonly property int spacingXXLarge: 48

  // ==================== Margins ====================
  readonly property int marginTiny: 4
  readonly property int marginSmall: 8
  readonly property int marginMedium: 16
  readonly property int marginLarge: 24
  readonly property int marginXLarge: 32
  readonly property int marginXXLarge: 48

  // ==================== Padding ====================
  readonly property int paddingTiny: 4
  readonly property int paddingSmall: 8
  readonly property int paddingMedium: 12
  readonly property int paddingLarge: 16
  readonly property int paddingXLarge: 24

  // ==================== Component Sizes ====================
  readonly property int touchButtonHeight: 48
  readonly property int buttonHeightSmall: 32
  readonly property int buttonHeightMedium: 40
  readonly property int buttonHeightLarge: 48
  readonly property int inputHeight: 40
  readonly property int inputHeightLarge: 48
  readonly property int iconButtonSize: 40
  readonly property int iconButtonSizeSmall: 32
  readonly property int iconButtonSizeLarge: 48
  readonly property int avatarSizeSmall: 32
  readonly property int avatarSizeMedium: 40
  readonly property int avatarSizeLarge: 56
  readonly property int toolbarHeight: 56
  readonly property int tabBarHeight: 48
  readonly property int statusBarHeight: 24

  // ==================== Navigation ====================
  readonly property int navigationBarWidth: 300
  readonly property int navigationRailWidth: 72
  readonly property int drawerWidth: 280
  readonly property int sidebarWidth: 320
  readonly property int sidebarMinWidth: 240
  readonly property int sidebarMaxWidth: 400

  // ==================== Border Radius ====================
  readonly property int radiusTiny: 2
  readonly property int radiusSmall: 4
  readonly property int radiusMedium: 8
  readonly property int radiusLarge: 12
  readonly property int radiusXLarge: 16
  readonly property int radiusRound: 9999

  // ==================== Border Width ====================
  readonly property int borderWidthThin: 1
  readonly property int borderWidthMedium: 2
  readonly property int borderWidthThick: 3

  // ==================== Elevation / Shadow ====================
  readonly property int elevationNone: 0
  readonly property int elevationLow: 2
  readonly property int elevationMedium: 4
  readonly property int elevationHigh: 8
  readonly property int elevationHighest: 16

  // ==================== Animation Duration ====================
  readonly property int animationInstant: 50
  readonly property int animationFast: 100
  readonly property int animationNormal: 200
  readonly property int animationSlow: 300
  readonly property int animationVerySlow: 500
  readonly property int touchAnimationDuration: 200

  // ==================== Z-Index ====================
  readonly property int zIndexBase: 0
  readonly property int zIndexDropdown: 100
  readonly property int zIndexSticky: 200
  readonly property int zIndexFixed: 300
  readonly property int zIndexOverlay: 400
  readonly property int zIndexModal: 500
  readonly property int zIndexPopover: 600
  readonly property int zIndexTooltip: 700
  readonly property int zIndexNotification: 800

  // ==================== Breakpoints ====================
  readonly property int breakpointMobile: 480
  readonly property int breakpointTablet: 768
  readonly property int breakpointDesktop: 1024
  readonly property int breakpointLarge: 1280
  readonly property int breakpointXLarge: 1536

  // ==================== Content Width ====================
  readonly property int contentMaxWidth: 1200
  readonly property int contentMaxWidthNarrow: 800
  readonly property int contentMaxWidthWide: 1400

  // ==================== List Item ====================
  readonly property int listItemHeightSmall: 40
  readonly property int listItemHeightMedium: 48
  readonly property int listItemHeightLarge: 56
  readonly property int listItemHeightTwoLine: 72

  // ==================== Card ====================
  readonly property int cardMinWidth: 200
  readonly property int cardMaxWidth: 400
  readonly property int cardPadding: 16

  // ==================== Dialog ====================
  readonly property int dialogMinWidth: 280
  readonly property int dialogMaxWidth: 560
  readonly property int dialogPadding: 24
}
