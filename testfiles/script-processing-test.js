import * as helpers from "helpers.js"

export function main(file) {
    utility.diag("debug", Object.keys(settings).join(", "), "settings");
    for (const tag of file.tags) {
        changeTagFields(file, tag);
    }
    file.applyChanges();
    return !helpers.isTruthy(settings.dryRun);
}

function addTestFields(file, tag) {
    const fields = tag.fields;
    for (const [key, value] of Object.entries(settings)) {
        if (key.startsWith("set:")) {
            fields[key.substr(4)] = value;
        }
    }
}

function changeTagFields(file, tag) {
    helpers.logTagInfo(file, tag);
    addTestFields(file, tag);
}
