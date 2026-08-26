import QtQuick
import QtQuick.Controls
import org.kde.kirigami as Kirigami

Kirigami.Dialog {
    id: dialog
    title: "Interface Info: " + interfaceName
    width: 500
    height: 350
    standardButtons: Dialog.Close

    property string interfaceName: ""
    property string infoText: "Fetching data..."

    contentItem: ScrollView {
        clip: true
        TextArea {
            readOnly: true
            text: dialog.infoText
            font.family: "monospace"
            font.pointSize: 10
            color: "#00cec9"
            selectByMouse: true
            wrapMode: TextArea.NoWrap
            background: Rectangle { color: "#2d3436" }
        }
    }
}
