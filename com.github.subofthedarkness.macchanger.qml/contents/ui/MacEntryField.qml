import QtQuick
import QtQuick.Controls
import org.kde.kirigami as Kirigami

TextField {
    id: control
    placeholderText: "AA:BB:CC:DD:EE:FF"
    inputMask: "HH:HH:HH:HH:HH:HH;_"

    Kirigami.Theme.colorSet: Kirigami.Theme.Window

    readonly property bool isValid: text.length === 17 && text.indexOf("_") === -1
    readonly property bool hasError: text.indexOf("_") !== -1 && text.replace(/[:_]/g, "").length > 0

    background: Rectangle {
        implicitWidth: 200
        implicitHeight: 32
        
        color: control.activeFocus ? Kirigami.Theme.backgroundColor : Kirigami.Theme.alternateBackgroundColor
        
        border.color: control.hasError 
                      ? "#ff4d4d" 
                      : (control.activeFocus ? Kirigami.Theme.focusColor : Kirigami.Theme.borderColor)
                      
        border.width: control.hasError || control.activeFocus ? 2 : 1
        radius: 4
    }

    onTextChanged: text = text.toUpperCase()
}
