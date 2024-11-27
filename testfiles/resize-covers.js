import * as helpers from "helpers.js"

export function main(file) {
    // read parameters from CLI arguments
    const format = settings.coverFormat ?? "JPEG"; // see https://doc.qt.io/qt-6/qimagewriter.html#supportedImageFormats
    const maxWidthAndHeight = parseInt(settings.coverSize) ?? 256; // maximum for width and height
    const maxSize = Qt.size(maxWidthAndHeight, maxWidthAndHeight);
    const quality = parseInt(settings.coverQuality) ?? -1; // see https://doc.qt.io/qt-6/qimagewriter.html#setQuality

    // resize/convert all covers in all tags of the file
    file.tags.forEach(tag => {
        const covers = tag.fields.cover;
        if (covers !== undefined) {
            // convert all covers where width and height exceeds maxSize keeping aspect ratio
            // keep all covers as-is that do not exceed maxSize or do not contain valid/supported image data
            tag.fields.cover = covers.map(cover => cover.content instanceof ArrayBuffer ? utility.convertImage(cover.content, maxSize, format) : cover.content);
        }
    });

    // submit changes from the JavaScript-context to the tag editor application; does not save changes to disk yet
    file.applyChanges();

    // return true to actually apply the changes or false to skip the file
    return !helpers.isTruthy(settings.dryRun);
}
