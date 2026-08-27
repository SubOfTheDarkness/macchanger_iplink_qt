import QtQuick
import QtQuick.Controls
import org.kde.kirigami as Kirigami

Kirigami.Dialog {
    id: dialog
    title: "Interface Info: " + interfaceName
    
    width: Math.min(500, parent ? parent.width - 20 : 500)
    height: Math.min(350, parent ? parent.height - 20 : 350)
    standardButtons: Dialog.Close

    property string interfaceName: ""
    property string infoText: "Fetching data..."

    contentItem: ScrollView {
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        ScrollBar.vertical.policy: ScrollBar.AsNeeded

        TextArea {
            readOnly: true
            text: dialog.infoText
            font.family: "monospace"
            font.pointSize: 10
            color: "#00cec9"
            selectByMouse: true
            
            wrapMode: TextArea.Wrap
            
            width: parent.width 
            
            background: Rectangle { color: "#2d3436" }
        }
    }
}
