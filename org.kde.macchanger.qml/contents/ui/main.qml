import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.plasma.plasmoid
import org.kde.plasma.components as PlasmaComponents
import org.kde.kirigami as Kirigami

PlasmoidItem {
    id: root
    
    width: 380
    height: 380

    property string currentMac: "Loading..."
    property string selectedInterface: ""
    property string usernameSystem: "" 
    property bool isUpdating: false
    property var defaultsMap: ({})

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

        onInterfacesLoaded: (list) => {
            var previousSelection = root.selectedInterface;
            
            ifaceModel.clear();
            for (var i = 0; i < list.length; i++) {
                ifaceModel.append({ "text": list[i] });
            }
            
            if (previousSelection !== "") {
                var targetIndex = 0;
                for (var j = 0; j < ifaceModel.count; j++) {
                    if (ifaceModel.get(j).text === previousSelection) {
                        targetIndex = j;
                        break;
                    }
                }
                ifaceDropbox.currentIndex = targetIndex;
            }
            console.log("[MACCHANGER_LOG] Interfaces reloaded. Restored selection to index: " + ifaceDropbox.currentIndex);
        }


        onProfilesLoaded: (profiles, defaults) => {
            root.defaultsMap = defaults;
            
            profileModel.clear();
            profileModel.append({ "name": "[Enter Custom]", "mac": "" });
            
            for (var i = 0; i < profiles.length; i++) {
                profileModel.append(profiles[i]);
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
            bashExecutor.connectSource(sedCmd);
            
            var dynamicConnector = Qt.createQmlObject(
                'import QtQuick; Connections {
                    target: bashExecutor;
                    function onProfilesLoaded() {
                        var newIndex = -1;
                        for (var j = 0; j < profileModel.count; j++) {
                            if (profileModel.get(j).name === "' + targetModelLabel + '") {
                                newIndex = j;
                                break;
                            }
                        }
                        if (newIndex !== -1) {
                            root.forceSelectIndex(newIndex);
                        }
                        this.destroy();
                    }
                }', root, "DynamicNativeResetHandler"
            );
            
            root.triggerConfigLoad();
        }
    }

    function executeMacChange(iface, newMac) {
        var script = "pkexec sh -c 'ip link set dev " + iface + " down && ip link set dev " + iface + " address " + newMac + " && ip link set dev " + iface + " up'";
        bashExecutor.connectSource(script);
    }

    function triggerConfigLoad() {
        bashExecutor.resetCache();
        
        var currentUiPath = Qt.resolvedUrl(".").toString().replace("file://", "");
        
        bashExecutor.connectSource("cat " + currentUiPath + "../config/default_config.ini 2>/dev/null");
        
        bashExecutor.connectSource("bash -c 'cat $HOME/.config/macchanger/address_aliases.ini 2>/dev/null'");
    }

    compactRepresentation: Kirigami.Icon { source: "macchanger-widget-icon"; active: root.expanded }

    fullRepresentation: Item {
        anchors.fill: parent

        Component.onCompleted: {
            profileModel.clear();
            profileModel.append({ "name": "[Enter Custom]", "mac": "" });
            profileDropbox.currentIndex = 0;

            bashExecutor.connectSource("ip -o link show");
            root.triggerConfigLoad();
        }


        InterfaceInfoDialog { 
            id: infoDialog 
            
            infoText: root.interfaceExtendedInfo

            Connections {
                target: root
                function onInterfaceInfoRequested() {
                    root.interfaceExtendedInfo = "Fetching data...";
                    bashExecutor.connectSource("ip addr show " + ifaceDropbox.currentText);
                    infoDialog.open();
                }
            }
        }



        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 15
            spacing: 8

            PlasmaComponents.Label { text: "Network Interface:"; font.bold: true }
            RowLayout {
                Layout.fillWidth: true
                spacing: 6
                
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
                    enabled: ifaceDropbox.currentText !== "" && !root.isUpdating
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

            PlasmaComponents.Label { text: "MAC Address Profiles:"; font.bold: true }
            RowLayout {
                Layout.fillWidth: true
                spacing: 6

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

            PlasmaComponents.Label { text: "Target MAC Address:"; font.bold: true }
            
            MacEntryField {
                id: macEntry
                Layout.fillWidth: true
                readOnly: profileDropbox.currentIndex !== 0 || root.isUpdating
                opacity: readOnly ? 0.6 : 1.0
            }

            Item { Layout.fillHeight: true }

            Kirigami.PromptDialog {
                id: confirmDialog
                title: "Confirmation"
                subtitle: "Change the MAC address to " + macEntry.text.trim() + " on interface " + ifaceDropbox.currentText + "?"
                standardButtons: Kirigami.Dialog.Yes | Kirigami.Dialog.No
                onAccepted: {
                    root.isUpdating = true;
                    root.executeMacChange(ifaceDropbox.currentText, macEntry.text.trim());
                }
            }

            PlasmaComponents.Button {
                Layout.fillWidth: true
                text: "Apply Changes"
                enabled: macEntry.isValid && ifaceDropbox.currentText !== "" && !root.isUpdating
                onClicked: confirmDialog.open()
            }
        }
    }

    Timer {
        id: updateTimer
        interval: 2000; repeat: true; running: root.expanded
        onTriggered: { if (root.selectedInterface) bashExecutor.connectSource("ip link show " + root.selectedInterface); }
    }
}
