pragma Singleton
import QtQuick

QtObject {
  // ==================== Font Families ====================
  readonly property string fontFamily: "Roboto"
  readonly property string fontFamilyMono: "Roboto Mono"
  readonly property string iconFontFamily: "Material Icons"

  // ==================== Font Sizes ====================
  readonly property int displayFontSize: 32
  readonly property int headlineFontSize: 28
  readonly property int titleFontSize: 24
  readonly property int subtitleFontSize: 18
  readonly property int bodyFontSize: 14
  readonly property int bodySmallFontSize: 13
  readonly property int captionFontSize: 12
  readonly property int smallFontSize: 10
  readonly property int iconFontSize: 24
  readonly property int iconSmallFontSize: 18
  readonly property int iconLargeFontSize: 32

  // ==================== Font Objects ====================
  readonly property font display: Qt.font({
    family: fontFamily,
    pixelSize: displayFontSize,
    weight: Font.Bold
  })

  readonly property font headline: Qt.font({
    family: fontFamily,
    pixelSize: headlineFontSize,
    weight: Font.Bold
  })

  readonly property font title: Qt.font({
    family: fontFamily,
    pixelSize: titleFontSize,
    weight: Font.Medium
  })

  readonly property font titleBold: Qt.font({
    family: fontFamily,
    pixelSize: titleFontSize,
    weight: Font.Bold
  })

  readonly property font subtitle: Qt.font({
    family: fontFamily,
    pixelSize: subtitleFontSize,
    weight: Font.Medium
  })

  readonly property font subtitleBold: Qt.font({
    family: fontFamily,
    pixelSize: subtitleFontSize,
    weight: Font.Bold
  })

  readonly property font body: Qt.font({
    family: fontFamily,
    pixelSize: bodyFontSize,
    weight: Font.Normal
  })

  readonly property font bodyBold: Qt.font({
    family: fontFamily,
    pixelSize: bodyFontSize,
    weight: Font.Bold
  })

  readonly property font bodySmall: Qt.font({
    family: fontFamily,
    pixelSize: bodySmallFontSize,
    weight: Font.Normal
  })

  readonly property font caption: Qt.font({
    family: fontFamily,
    pixelSize: captionFontSize,
    weight: Font.Normal
  })

  readonly property font captionBold: Qt.font({
    family: fontFamily,
    pixelSize: captionFontSize,
    weight: Font.Bold
  })

  readonly property font small: Qt.font({
    family: fontFamily,
    pixelSize: smallFontSize,
    weight: Font.Normal
  })

  readonly property font mono: Qt.font({
    family: fontFamilyMono,
    pixelSize: bodyFontSize,
    weight: Font.Normal
  })

  readonly property font monoBold: Qt.font({
    family: fontFamilyMono,
    pixelSize: bodyFontSize,
    weight: Font.Bold
  })

  // ==================== Icon Fonts ====================
  readonly property font icon: Qt.font({
    family: iconFontFamily,
    pixelSize: iconFontSize,
    weight: Font.Normal
  })

  readonly property font iconSmall: Qt.font({
    family: iconFontFamily,
    pixelSize: iconSmallFontSize,
    weight: Font.Normal
  })

  readonly property font iconLarge: Qt.font({
    family: iconFontFamily,
    pixelSize: iconLargeFontSize,
    weight: Font.Normal
  })
}
