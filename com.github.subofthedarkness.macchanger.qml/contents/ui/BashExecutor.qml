import QtQuick
import org.kde.plasma.plasma5support as Plasma5Support
import "ConfigParser.js" as ConfigParser

Plasma5Support.DataSource {
    id: executor
    engine: "executable"
    connectedSources: []

    signal interfacesLoaded(var interfaceList)
    signal profilesLoaded(var profilesList, var defaultsMap)
    signal currentMacLoaded(string macAddress)
    signal extendedInfoLoaded(string infoText)
    
    signal errorOccurred(string errorText)
    signal dependenciesChecked(bool ipFound, bool sedFound, bool pkexecFound)

    property var cachedDefaultProfiles: []
    property var cachedUserProfiles: []
    property var cachedDefaultsMap: ({})

    function resetCache() {
        executor.cachedDefaultProfiles = [];
        executor.cachedUserProfiles = [];
        executor.cachedDefaultsMap = ({});
        console.log("[MACCHANGER_LOG] Cache has been cleared successfully.");
    }

    function mergeAndEmit() {
        var finalProfiles = [];
        var processedNames = {};

        for (var i = 0; i < executor.cachedUserProfiles.length; i++) {
            var pUser = executor.cachedUserProfiles[i];
            finalProfiles.push(pUser);
            processedNames[pUser.name] = true;
        }

        for (var j = 0; j < executor.cachedDefaultProfiles.length; j++) {
            var pDef = executor.cachedDefaultProfiles[j];
            if (!processedNames[pDef.name]) {
                finalProfiles.push(pDef);
            }
        }

        console.log("[MACCHANGER_LOG] Merged total profiles count: " + finalProfiles.length);
        executor.profilesLoaded(finalProfiles, executor.cachedDefaultsMap);
    }

    onNewData: (sourceName, data) => {
        var stdout = data.stdout ? data.stdout.toString() : "";
        var stderr = data.stderr ? data.stderr.toString() : "";
        var output = stdout.trim();
        
        console.log("[MACCHANGER_LOG] Incoming data from: " + sourceName + " | Output bytes: " + output.length);
        
        if (sourceName.indexOf("which ip") === 0) {
            var lines = output.split('\n');
            var ipFound = false;
            var sedFound = false;
            var pkexecFound = false;
            
            for (var k = 0; k < lines.length; k++) {
                if (lines[k].indexOf("/ip") !== -1) ipFound = true;
                if (lines[k].indexOf("/sed") !== -1) sedFound = true;
                if (lines[k].indexOf("/pkexec") !== -1) pkexecFound = true;
            }
            
            executor.dependenciesChecked(ipFound, sedFound, pkexecFound);
            executor.disconnectSource(sourceName);
            return;
        }

        if (stderr.length > 0) {
            console.log("[MACCHANGER_ERROR] " + stderr);
            if (sourceName.indexOf("pkexec") !== -1) {
                executor.errorOccurred("Ошибка прав доступа или отмена операции.");
            } else {
                executor.errorOccurred(stderr.trim());
            }
            executor.disconnectSource(sourceName);
            return;
        }
        
        if (sourceName.indexOf("ip -o link show") !== -1) {
            var lines = output.split('\n');
            var parsedList = [];
            for (var i = 0; i < lines.length; i++) {
                var ifaceLine = lines[i].trim();
                if (!ifaceLine) continue;
                var firstColon = ifaceLine.indexOf(":");
                if (firstColon !== -1) {
                    var restOfLine = ifaceLine.substring(firstColon + 1).trim();
                    var secondColon = restOfLine.indexOf(":");
                    if (secondColon !== -1) {
                        var name = restOfLine.substring(0, secondColon).trim();
                        if (name && name !== "lo") parsedList.push(name);
                    }
                }
            }
            executor.interfacesLoaded(parsedList);
        }
        
        if (sourceName.indexOf(".ini") !== -1) {
            var isUser = sourceName.indexOf("address_aliases.ini") !== -1;
            console.log("[MACCHANGER_LOG] Parsing file. Is user custom file? -> " + isUser);

            var result = ConfigParser.parseIni(output);
            console.log("[MACCHANGER_LOG] Extracted from text: " + result.profiles.length + " profiles");

            for (var key in result.defaults) {
                executor.cachedDefaultsMap[key] = result.defaults[key];
            }

            if (isUser) {
                executor.cachedUserProfiles = result.profiles;
            } else {
                executor.cachedDefaultProfiles = result.profiles;
            }

            if (executor.cachedDefaultProfiles.length > 0 || isUser) {
                executor.mergeAndEmit();
            }
        }
        
        if (sourceName.indexOf("ip link show ") === 0) {
            var macIndex = output.indexOf("link/ether");
            if (macIndex !== -1) {
                var macPart = output.substring(macIndex + 10).trim();
                var spaceIndex = macPart.indexOf(" ");
                var finalMac = (spaceIndex !== -1) ? macPart.substring(0, spaceIndex) : macPart;
                executor.currentMacLoaded(finalMac.trim().toUpperCase());
            } else {
                executor.currentMacLoaded("Not specified / Dynamic");
            }
        }

        if (sourceName.indexOf("ip addr show ") === 0) {
            executor.extendedInfoLoaded(output ? output : "Extended information could not be obtained.");
        }
        
        executor.disconnectSource(sourceName);
    }
}
