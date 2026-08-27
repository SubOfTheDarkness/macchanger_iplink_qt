import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.plasma.plasmoid
import org.kde.plasma.components as PlasmaComponents
import org.kde.kirigami as Kirigami
import "ConfigParser.js" as ConfigParser

PlasmoidItem {
    id: root
    
    width: 380
    height: 420

    property string currentMac: "Loading..."
    property string selectedInterface: ""
    property string usernameSystem: "" 
    property bool isUpdating: false
    property var defaultsMap: ({})

    property bool hasDependencies: true
    property string errorAlertText: ""

    property string pendingProfileSelection: ""
    property bool configLoaded: false

    signal forceSelectIndex(int index)
    signal interfaceInfoRequested()
    property string interfaceExtendedInfo: "Fetching data..."

    ListModel { id: ifaceModel }
    ListModel { 
        id: profileModel 
        ListElement { name: "[Enter Custom]"; mac: "" }
    }

    BashExecutor {
        id: bashExecutor

        onErrorOccurred: (errorText) => {
            root.isUpdating = false;
            root.errorAlertText = errorText;
        }

        onDependenciesChecked: (ipFound, sedFound, pkexecFound) => {
            if (!ipFound || !sedFound || !pkexecFound) {
                root.hasDependencies = false;
                var missing = [];
                if (!ipFound) missing.push("iproute2 (ip)");
                if (!sedFound) missing.push("sed");
                if (!pkexecFound) missing.push("polkit (pkexec)");
                root.errorAlertText = "В системе нет нужного софта: " + missing.join(", ");
            }
        }

        onInterfacesLoaded: (list) => {
            var previousSelection = root.selectedInterface;
            
            ifaceModel.clear();
            for (var i = 0; i < list.length; i++) {
                ifaceModel.append({ "text": list[i] });
            }
            
            var targetIndex = 0;
            if (previousSelection !== "") {
                for (var j = 0; j < ifaceModel.count; j++) {
                    if (ifaceModel.get(j).text === previousSelection) {
                        targetIndex = j;
                        break;
                    }
                }
                if (ifaceDropbox) {
                    ifaceDropbox.currentIndex = targetIndex;
                }
            }
            console.log("[MACCHANGER_LOG] Interfaces reloaded. Restored selection to index: " + targetIndex);
        }

        onProfilesLoaded: (profiles, defaults) => {
            root.defaultsMap = defaults;
            
            profileModel.clear();
            profileModel.append({ "name": "[Enter Custom]", "mac": "" });
            
            for (var i = 0; i < profiles.length; i++) {
                var uiName = profiles[i].name.replace(/_/g, " ");
                profileModel.append({ "name": uiName, "mac": profiles[i].mac });
            }
        }

        onAllProfilesReady: {
            root.configLoaded = true;
            
            if (root.pendingProfileSelection !== "") {
                var targetIdx = ConfigParser.findIndexInModel(profileModel, root.pendingProfileSelection);
                if (targetIdx !== -1) {
                    root.forceSelectIndex(targetIdx);
                } else {
                    root.forceSelectIndex(0);
                }
                root.pendingProfileSelection = "";
            }
        }

        onCurrentMacLoaded: (mac) => {
            root.currentMac = mac;
            root.isUpdating = false;
        }

        onExtendedInfoLoaded: (text) => {
            root.interfaceExtendedInfo = text;
        }
    }

    onSelectedInterfaceChanged: {
        if (root.selectedInterface) {
            root.currentMac = "Loading...";
            bashExecutor.connectSource("ip link show " + root.selectedInterface);
        }
    }

    function resetToNative() {
        if (!root.selectedInterface) return;
        
        var nativeMac = root.defaultsMap[root.selectedInterface];
        if (!nativeMac) {
            console.log("[MACCHANGER] In the config, under the [DEFAULTS] section, no address for " + root.selectedInterface + " was found.");
            return;
        }
        
        var targetModelLabel = "default " + root.selectedInterface;
        var targetDiskLabel = "default_" + root.selectedInterface;
        var existIndex = -1;
        
        for (var i = 0; i < profileModel.count; i++) {
            if (profileModel.get(i).name === targetModelLabel) {
                existIndex = i;
                break;
            }
        }
        
        if (existIndex !== -1) {
            root.forceSelectIndex(existIndex);
        } else {
            var sedCmd = "bash -c 'sed -i \"/\\[MAC_ALIASES\\]/a " + targetDiskLabel + " = " + nativeMac + "\" $HOME/.config/macchanger/address_aliases.ini'";
            
            console.log("[MACCHANGER] Writing new profile via native $HOME sed: " + targetDiskLabel);
            
            root.pendingProfileSelection = targetModelLabel;
            
            bashExecutor.connectSource(sedCmd);
            root.triggerConfigLoad();
        }
    }

    function executeMacChange(iface, newMac) {
        var script = "pkexec sh -c 'ip link set dev " + iface + " down && ip link set dev " + iface + " address " + newMac + " && ip link set dev " + iface + " up'";
        bashExecutor.connectSource(script);
    }

    function triggerConfigLoad() {
        root.configLoaded = false;
        bashExecutor.resetCache();
        var currentUiPath = Qt.resolvedUrl(".").toString().replace("file://", "");
        bashExecutor.connectSource("cat " + currentUiPath + "../config/default_config.ini 2>/dev/null");
        bashExecutor.connectSource("bash -c 'cat $HOME/.config/macchanger/address_aliases.ini 2>/dev/null'");
    }

    compactRepresentation: PlasmaComponents.Button {
        id: compactButton
        Layout.preferredWidth: Kirigami.Units.gridUnit * 2
        Layout.preferredHeight: Kirigami.Units.gridUnit * 2
        
        contentItem: Kirigami.Icon {
            source: "macchanger-widget-icon"
            active: compactButton.hovered || root.expanded
        }
        onClicked: {
            root.expanded = !root.expanded;
        }
    }
    fullRepresentation: Item {
        anchors.fill: parent

        Component.onCompleted: {
            profileModel.clear();
            profileModel.append({ "name": "[Enter Custom]", "mac": "" });
            if (profileDropbox) {
                profileDropbox.currentIndex = 0;
            }
            
            bashExecutor.connectSource("which ip sed pkexec");
            bashExecutor.connectSource("ip -o link show");
            root.triggerConfigLoad();
        }

        InterfaceInfoDialog { 
            id: infoDialog 
            infoText: root.interfaceExtendedInfo
            interfaceName: root.selectedInterface

            Connections {
                target: root
                function onInterfaceInfoRequested() {
                    root.interfaceExtendedInfo = "Fetching data...";
                    bashExecutor.connectSource("ip addr show " + root.selectedInterface);
                    infoDialog.open();
                }
            }
        }

        Kirigami.PromptDialog {
            id: saveProfileDialog
            title: "Save Profile"
            subtitle: "Enter a name for the profile with address " + macEntry.text.trim().toUpperCase() + ":"
            standardButtons: Kirigami.Dialog.Ok | Kirigami.Dialog.Cancel
            
            contentItem: TextField {
                id: profileNameInput
                placeholderText: "e.g.: Home Router"
                Layout.fillWidth: true
            }

            onAccepted: {
                var inputName = profileNameInput.text.trim();
                if (inputName === "") return;

                var diskKey = inputName.replace(/ /g, "_");
                var targetMac = macEntry.text.trim().toUpperCase();

                var sedCmd = "bash -c 'sed -i \"/\\[MAC_ALIASES\\]/a " + diskKey + " = " + targetMac + "\" $HOME/.config/macchanger/address_aliases.ini'";
                
                console.log("[MACCHANGER] Saving custom profile via sed: " + diskKey + " = " + targetMac);
                bashExecutor.connectSource(sedCmd);
                root.pendingProfileSelection = inputName;
                root.triggerConfigLoad();
                profileNameInput.text = "";
            }
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 15
            spacing: 8

            Kirigami.InlineMessage {
                id: msgAlert
                Layout.fillWidth: true
                type: root.hasDependencies ? Kirigami.MessageType.Error : Kirigami.MessageType.Warning
                text: root.errorAlertText
                visible: root.errorAlertText !== ""
                showCloseButton: root.hasDependencies
                onVisibleChanged: {
                    if (!visible && root.hasDependencies) {
                        root.errorAlertText = ""; 
                    }
                }
            }

            PlasmaComponents.Label { 
                text: "Network Interface:"
                font.bold: true 
                opacity: root.hasDependencies ? 1.0 : 0.5
            }
            
            RowLayout {
                Layout.fillWidth: true
                spacing: 6
                enabled: root.hasDependencies && !root.isUpdating
                
                ComboBox {
                    id: ifaceDropbox
                    Layout.fillWidth: true
                    model: ifaceModel
                    textRole: "text"
                    currentIndex: 0
                    enabled: !root.isUpdating

                    Binding { 
                        target: root
                        property: "selectedInterface"
                        value: ifaceDropbox.currentIndex !== -1 ? ifaceDropbox.currentText : root.selectedInterface
                    }
                }

                PlasmaComponents.Button {
                    text: "Info"
                    enabled: ifaceDropbox.currentText !== "" && !root.isUpdating
                    onClicked: { 
                        root.interfaceInfoRequested();
                    }
                }

                PlasmaComponents.Button {
                    text: "Reset to native MAC"
                    enabled: ifaceDropbox.currentText !== "" && !root.isUpdating && root.configLoaded
                    onClicked: root.resetToNative()
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                PlasmaComponents.Label { text: "Current MAC: " }
                PlasmaComponents.Label {
                    text: root.currentMac 
                    font.bold: true
                    color: root.isUpdating ? "#557f77" : "#00ffcc"
                }
                PlasmaComponents.BusyIndicator {
                    Layout.preferredWidth: 16; Layout.preferredHeight: 16
                    running: root.isUpdating; visible: root.isUpdating
                }
            }

            PlasmaComponents.Label { 
                text: "MAC Address Profiles:"
                font.bold: true 
                opacity: root.hasDependencies ? 1.0 : 0.5
            }
            
            RowLayout {
                Layout.fillWidth: true
                spacing: 6
                enabled: root.hasDependencies && !root.isUpdating

                ComboBox {
                    id: profileDropbox
                    Layout.fillWidth: true
                    model: profileModel
                    textRole: "name" 
                    currentIndex: 0
                    enabled: !root.isUpdating
                    
                    onCountChanged: {
                        currentIndex = 0;
                        macEntry.text = "";
                    }
                    Connections {
                        target: root
                        function onForceSelectIndex(index) {
                            profileDropbox.currentIndex = index;
                            var selectedMac = profileModel.get(index).mac;
                            macEntry.text = (selectedMac !== "") ? selectedMac : "";
                        }
                    }
                    Connections {
                        target: root
                        function onSelectedInterfaceChanged() { 
                            profileDropbox.currentIndex = 0; 
                            macEntry.text = ""; 
                        }
                    }

                    onActivated: {
                        var selectedMac = model.get(currentIndex).mac;
                        macEntry.text = (selectedMac !== "") ? selectedMac : "";
                    }
                }

                PlasmaComponents.Button {
                    text: "Reload"
                    enabled: !root.isUpdating
                    onClicked: {
                        root.triggerConfigLoad();
                        bashExecutor.connectSource("notify-send -a 'MACChanger' -i 'macchanger-widget-icon' 'MACChanger' 'Profiles reloaded successfully.'");
                    }
                }
            }

            PlasmaComponents.Label { 
                text: "Target MAC Address:"
                font.bold: true 
                opacity: root.hasDependencies ? 1.0 : 0.5
            }
            
            RowLayout {
                Layout.fillWidth: true
                spacing: 6

                MacEntryField {
                    id: macEntry
                    Layout.fillWidth: true
                    readOnly: profileDropbox.currentIndex !== 0 || root.isUpdating || !root.hasDependencies
                    opacity: readOnly ? 0.6 : 1.0
                }

                PlasmaComponents.Button {
                    text: "Rand"
                    visible: profileDropbox.currentIndex === 0
                    enabled: root.hasDependencies && !root.isUpdating
                    onClicked: {
                        macEntry.text = root.generateRandomMac();
                    }
                }

                PlasmaComponents.Button {
                    text: "Save"
                    visible: profileDropbox.currentIndex === 0
                    enabled: macEntry.isValid && root.hasDependencies && !root.isUpdating
                    onClicked: {
                        saveProfileDialog.open();
                    }
                }
            }

            Item { Layout.fillHeight: true }

            Kirigami.PromptDialog {
                id: confirmDialog
                title: "Confirmation"
                subtitle: "Change the MAC address to " + macEntry.text.trim() + " on interface " + root.selectedInterface + "?"
                standardButtons: Kirigami.Dialog.Yes | Kirigami.Dialog.No
                onAccepted: {
                    root.isUpdating = true;
                    root.executeMacChange(root.selectedInterface, macEntry.text.trim());
                }
            }

            PlasmaComponents.Button {
                Layout.fillWidth: true
                text: "Apply Changes"
                enabled: macEntry.isValid && root.selectedInterface !== "" && !root.isUpdating && root.hasDependencies
                onClicked: confirmDialog.open()
            }
            RowLayout {
                Layout.fillWidth: true
                PlasmaComponents.Label {
                    text: "MacChanger QML v1.0 by sub"
                    font.pointSize: 9
                    opacity: 0.4
                    Layout.alignment: Qt.AlignLeft
                }
                
                Item { Layout.fillWidth: true }
                
                Kirigami.LinkButton {
                    text: "GitHub"
                    font.pointSize: 9
                    opacity: 0.6
                    Layout.alignment: Qt.AlignRight
                    onClicked: Qt.openUrlExternally("https://github.com/SubOfTheDarkness/macchanger_iplink_qt")
                }
            }
        }
    }

    function generateRandomMac() {
        var firstBytes = ["02", "06", "0A", "0E", "12", "16", "1A", "1E"];
        var mac = [firstBytes[Math.floor(Math.random() * firstBytes.length)]];
        for (var i = 0; i < 5; i++) {
            var byte = Math.floor(Math.random() * 256).toString(16).padStart(2, '0').toUpperCase();
            mac.push(byte);
        }
        return mac.join(":");
    }

    Timer {
        id: updateTimer
        interval: 2000; repeat: true; running: root.expanded && root.hasDependencies
        onTriggered: { if (root.selectedInterface) bashExecutor.connectSource("ip link show " + root.selectedInterface); }
    }
}
