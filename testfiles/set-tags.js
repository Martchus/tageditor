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

function changeTagFields(tag) {
    // log supported fields
    const fields = tag.fields;
    utility.diag("debug", tag.type, "tag");
    utility.diag("debug", Object.keys(fields).join(", "), "supported fields");

    // log tag type and fields for debugging purposes
    for (const [key, value] of Object.entries(fields)) {
        const content = value.content;
        if (content !== undefined && content != null && !(content instanceof ArrayBuffer)) {
            utility.diag("debug", content, key + " (" + value.type + ")");
        }
    }

    // change some fields
    fields.title.content = "foo";
    fields.artist.content = "bar";
    fields.track.content = "4/17";
    fields.comment.clear();
}
