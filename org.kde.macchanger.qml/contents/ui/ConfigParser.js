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
            defaultsMap[rawKey] = rawVal.toUpperCase();
        }

        if (activeSection === "MAC_ALIASES") {
            var cleanName = rawKey.replace(/_/g, " "); 
            var cleanMac = rawVal.toUpperCase();
            if (cleanName && cleanMac.indexOf(":") !== -1) {
                profilesList.push({ "name": cleanName, "mac": cleanMac });
            }
        }
    }

    return {
        profiles: profilesList,
        defaults: defaultsMap
    };
}
