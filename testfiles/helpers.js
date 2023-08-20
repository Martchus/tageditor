export function isString(value) {
    return typeof(value) === "string" || value instanceof String;
}

export function isTruthy(value) {
    return value && value !== "false" && value !== "0";
}

export function logTagInfo(file, tag) {
    // log tag type and supported fields
    const fields = tag.fields;
    utility.diag("debug", tag.type, "tag");
    utility.diag("debug", Object.keys(fields).sort().join(", "), "supported fields");

    // log fields for debugging purposes
    for (const [key, value] of Object.entries(fields)) {
        const content = value.content;
        if (content !== undefined && content != null && !(content instanceof ArrayBuffer)) {
            utility.diag("debug", content, key + " (" + value.type + ")");
        }
    }
}
