// import another module as an example how imports work
import * as http from "http.js"

export function main(file) {
    // iterate though all tags of the file to change fields in all of them
    for (const tag of file.tags) {
        changeTagFields(file, tag);
    }

    // submit changes from the JavaScript-context to the tag editor application; does not save changes to disk yet
    file.applyChanges();

    // return a falsy value to skip the file after all
    return false;
}

const mainTextFields = ["title", "artist", "album"];
const personalFields = ["comment", "rating"];

function isString(value) {
    return typeof(value) === "string" || value instanceof String;
}

function changeTagFields(file, tag) {
    // log tag type and supported fields
    const fields = tag.fields;
    utility.diag("debug", tag.type, "tag");
    utility.diag("debug", Object.keys(fields).join(", "), "supported fields");

    // log fields for debugging purposes
    for (const [key, value] of Object.entries(fields)) {
        const content = value.content;
        if (content !== undefined && content != null && !(content instanceof ArrayBuffer)) {
            utility.diag("debug", content, key + " (" + value.type + ")");
        }
    }

    // apply fixes to main text fields
    for (const key of mainTextFields) {
        for (const value of fields[key]) {
            if (isString(value.content)) {
                value.content = value.content.trim();
                value.content = utility.fixUmlauts(value.content);
                value.content = utility.formatName(value.content);
            }
        }
    }

    // ensure personal fields are cleared
    for (const key of personalFields) {
        fields[key] = [];
    }

    // set total number of tracks if not already assigned using the number of files in directory
    const track = fields.track;
    if (track.find(value => !value.content.total) !== undefined) {
        const extension = file.extension;
        const dirItems = utility.readDirectory(file.containingDirectory) || [];
        const total = dirItems.filter(fileName => fileName.endsWith(extension)).length;
        if (total) {
            for (const value of track) {
                value.content.total |= total;
            }
        }
    }

    // assume the number of disks is always one for now
    if (!fields.disk.length) {
        fields.disk = "1/1";
    }
}
