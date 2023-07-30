// import another module as an example how imports work
import * as http from "http.js"

export function main(file) {
    // iterate though all tags of the file to change fields in all of them
    for (const tag of file.tags) {
        changeTagFields(tag);
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

function changeTagFields(tag) {
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

    // set some other fields
    fields.track.content = "4/17";
}
