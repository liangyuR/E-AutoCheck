import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material

Item {
    id: networkConfigPane

    property int connectionTimeout: 30
    property bool connectionTimeout_visible: true
    property string ip: "127.0.0.1"
    property int port: 59999
    property bool port_visible: true

    signal configChanged(string ip, int port, int timeout)

    ColumnLayout {
        anchors.fill: parent
        spacing: layoutStyle.spacingMedium

        Label {
            font: fontStyle.subtitle
            text: "网络配置"
        }

        // IP地址配置
        RowLayout {
            Layout.fillWidth: true

            Label {
                font: fontStyle.body
                text: "连接配置"
            }
            TextField {
                id: ipTextField

                Layout.preferredWidth: 200
                horizontalAlignment: Text.AlignHCenter
                inputMethodHints: Qt.ImhDigitsOnly
                placeholderText: "请输入IP地址"
                text: ip

                validator: RegularExpressionValidator {
                    regularExpression: /^(\d{1,3}\.){3}\d{1,3}$/
                }

                Keys.onPressed: function (event) {
                    if (event.key === Qt.Key_Period || event.key === Qt.Key_Comma) {
                        event.accepted = true;
                        let pos = cursorPosition;
                        let newText = text.substring(0, pos) + "." + text.substring(pos);
                        text = newText;
                        cursorPosition = pos + 1;
                    }
                }
                onTextChanged: {
                    if (text.length > 0) {
                        let cleaned = text.replace(/[^\d.]/g, '');
                        let parts = cleaned.split('.');
                        for (let i = 0; i < parts.length; i++) {
                            // 限制每段最大为255
                            if (parts[i].length > 3) {
                                parts[i] = parts[i].substring(0, 3);
                            }
                            if (parseInt(parts[i]) > 255) {
                                parts[i] = "255";
                            }
                        }
                        let result = parts.join('.');
                        if (result !== text) {
                            ipTextField.text = result;
                            ipTextField.cursorPosition = result.length;
                        }
                        if (ipTextField.text !== ip) {
                            ip = ipTextField.text;
                            configChanged(ip, port, connectionTimeout);
                        }
                    }
                }
            }
            SpinBox {
                id: portSpinBox

                Layout.preferredWidth: 200
                editable: true
                from: 1
                to: 65535
                value: port

                onValueChanged: {
                    if (value !== port) {
                        port = value;
                        configChanged(ip, port, connectionTimeout);
                    }
                }
            }
            Item {
                Layout.fillWidth: true
            }
            Label {
                font: fontStyle.body
                text: "连接超时"
            }
            SpinBox {
                id: connectionTimeoutSpinBox

                Layout.preferredWidth: 200
                editable: true
                from: 1
                to: 60
                value: connectionTimeout

                onValueChanged: {
                    if (value !== connectionTimeout) {
                        connectionTimeout = value;
                        configChanged(ip, port, connectionTimeout);
                    }
                }
            }
        }
    }
}
