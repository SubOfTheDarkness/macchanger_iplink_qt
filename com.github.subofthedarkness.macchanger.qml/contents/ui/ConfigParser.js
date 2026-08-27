function decodeQtString(str) {
    if (!str) return "";
    
    return str.replace(/%U([0-9A-Fa-f]{4})/g, function(match, hex) {
        return String.fromCharCode(parseInt(hex, 16));
    });
}

function parseIni(rawText) {
    var lines = rawText.split('\n');
    var activeSection = "";
    var profilesList = [];
    var defaultsMap = {};

    for (var i = 0; i < lines.length; i++) {
        var line = lines[i].trim();
        if (!line || line.indexOf("#") === 0 || line.indexOf(";") === 0) continue;

        if (line.indexOf("[") === 0 && line.indexOf("]") !== -1) {
            activeSection = line.replace("[", "").replace("]", "").trim();
            continue;
        }

        if (line.indexOf("=") === -1) continue;

        var eqIndex = line.indexOf("=");
        var rawKey = line.substring(0, eqIndex).trim();
        var rawVal = line.substring(eqIndex + 1).trim().replace(/['\"]/g, "");

        if (rawKey.indexOf("HELP") !== -1) continue;

        if (activeSection === "DEFAULTS") {
            defaultsMap[rawKey.toLowerCase()] = rawVal.toUpperCase();
        }

        if (activeSection === "MAC_ALIASES") {
            var rawProfileName = decodeQtString(rawKey); 
            var cleanMac = rawVal.toUpperCase();
            if (rawProfileName && cleanMac.indexOf(":") !== -1) {
                profilesList.push({ "name": rawProfileName, "mac": cleanMac });
            }
        }
    }

    return {
        profiles: profilesList,
        defaults: defaultsMap
    };
}

function findIndexInModel(model, targetName) {
    if (!model || !targetName) return -1;
    var idx = -1;
    for (var i = 0; i < model.count; i++) {
        if (model.get(i).name === targetName) {
            idx = i;
            break;
        }
    }
    console.log("[macchanger ModelFinder] Find idx: " + idx + " for name: " + targetName);
    return idx;
}
