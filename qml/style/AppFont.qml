pragma Singleton
import QtQuick

QtObject {
    // Font families
    readonly property string fontFamily: "Roboto"
    readonly property string iconFontFamily: "Material Icons"

    // Font sizes
    readonly property int titleFontSize: 24
    readonly property int subtitleFontSize: 18
    readonly property int bodyFontSize: 14
    readonly property int captionFontSize: 12

    // Font objects
    readonly property font title: Qt.font({
        family: fontFamily,
        pixelSize: titleFontSize,
        weight: Font.Medium
    })

    readonly property font subtitle: Qt.font({
        family: fontFamily,
        pixelSize: subtitleFontSize,
        weight: Font.Medium
    })

    readonly property font body: Qt.font({
        family: fontFamily,
        pixelSize: bodyFontSize,
        weight: Font.Normal
    })

    readonly property font caption: Qt.font({
        family: fontFamily,
        pixelSize: captionFontSize,
        weight: Font.Normal
    })
}

