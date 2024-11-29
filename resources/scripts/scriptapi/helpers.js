const fileCache = {};

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
    for (const [key, values] of Object.entries(fields)) {
        for (const value of values) {
            const content = value.content;
            if (content !== undefined && content !== null && !(content instanceof ArrayBuffer)) {
                utility.diag("debug", content, key + " (" + value.type + ")");
            }
        }
    }
}

export function readFieldContents(file, fieldName) {
    const values = [];
    for (const tag of file.tags) {
        if (tag.type === "ID3v1 tag") {
            // due to its limitations ID3v1 tags may have truncated contents; so just ignore them
            // for the sake of this script
            continue;
        }
        for (const value of tag.fields[fieldName]) {
            const content = value.content;
            if (content !== undefined && content !== null) {
                values.push(content);
            }
        }
        if (values.length) {
            // just return the contents from the first tag that has any for now
            break;
        }
    }
    return values;
}

export function cacheValue(cache, key, generator) {
    const cachedValue = cache[key];
    return cachedValue ? cachedValue : (cache[key] = generator());
}

export function openOriginalFile(file) {
    const originalDir = settings.originalDir;
    const originalExt = settings.originalExt || file.extension;
    if (originalDir && file.pathRelative) {
        const name = file.nameWithoutExtension;
        const path = [originalDir, file.containingDirectory, name + originalExt].join("/");
        return cacheValue(fileCache, path, () => utility.openFile(path));
    }
}
