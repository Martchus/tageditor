import * as helpers from "helpers.js"
import * as metadatasearch from "metadatasearch.js"

export function main(file) {
    // iterate though all tags of the file to change fields in all of them
    for (const tag of file.tags) {
        changeTagFields(file, tag);
    }

    // submit changes from the JavaScript-context to the tag editor application; does not save changes to disk yet
    file.applyChanges();

    // return true to actually apply the changes or false to skip the file
    return !helpers.isTruthy(settings.dryRun);
}

const mainTextFields = ["title", "artist", "album", "genre"];
const personalFields = ["comment", "rating"];

function applyFixesToMainFields(file, tag) {
    const fields = tag.fields;
    for (const key of mainTextFields) {
        for (const value of fields[key]) {
            if (helpers.isString(value.content)) {
                value.content = value.content.trim();
                if (helpers.isTruthy(settings.fixUmlauts)) {
                    value.content = utility.fixUmlauts(value.content);
                }
                if (helpers.isTruthy(settings.formatNames)) {
                    value.content = utility.formatName(value.content);
                }
            }
        }
    }
}

function clearPersonalFields(file, tag) {
    const fields = tag.fields;
    for (const key of personalFields) {
        fields[key] = [];
    }
}

function addTotalNumberOfTracks(file, tag) {
    const track = tag.fields.track;
    if (track.find(value => !value.content.total) === undefined) {
        return; // skip if already assigned
    }
    const extension = file.extension;
    const dirItems = utility.readDirectory(file.containingDirectory) || [];
    const total = dirItems.filter(fileName => fileName.endsWith(extension)).length;
    if (!total) {
        return;
    }
    for (const value of track) {
        value.content.total |= total;
    }
}

function addFieldFromOriginalFile(file, tag, fieldName) {
    const originalFile = helpers.openOriginalFile(file);
    if (!originalFile) {
        return false;
    }
    utility.diag("debug", "Trying to take over " + fieldName + " from \"" + originalFile.path + "\".");
    const contents = helpers.readFieldContents(originalFile, fieldName);
    if (!contents.length) {
        utility.diag("debug", "No " + fieldName + " found in original file.");
        return false;
    }
    tag.fields[fieldName] = contents;
    return true;
}

function addLyrics(file, tag) {
    const fields = tag.fields;
    if (!fields.lyrics || fields.lyrics.length) {
        return; // skip if not supported by tag format or already assigned
    }
    if (addFieldFromOriginalFile(file, tag, "lyrics")) {
        return; // skip fetching via meta-data search if lyrics could be taken over from original file
    }
    const firstTitle = fields.title?.[0]?.content;
    const firstArtist = fields.artist?.[0]?.content;
    if (firstTitle && firstArtist) {
        fields.lyrics = metadatasearch.queryLyrics({title: firstTitle, artist: firstArtist});
    }
}

function addCover(file, tag) {
    const fields = tag.fields;
    if (!fields.cover || fields.cover.length) {
        return; // skip if not supported by tag format or already assigned
    }
    if (addFieldFromOriginalFile(file, tag, "cover")) {
        // ensure the cover's resolution is below a certain size to avoid bloating the file
        const convertedCovers = [];
        const maxSizeInt = parseInt(settings.coverMaxSize || 512);
        const maxSize = Qt.size(maxSizeInt, maxSizeInt);
        for (const cover of fields.cover) {
            convertedCovers.push(utility.convertImage(cover, maxSize));
        }
        fields.cover = convertedCovers;
        return; // skip fetching via meta-data search if cover could be taken over from original file
    }
    const firstAlbum = fields.album?.[0]?.content?.replace(/ \(.*\)/, '');
    const firstArtist = fields.artist?.[0]?.content;
    if (firstAlbum && firstArtist) {
        fields.cover = metadatasearch.queryCover({album: firstAlbum, artist: firstArtist});
    }
}

function addMiscFields(file, tag) {
    // assume the number of disks is always one for now
    const fields = tag.fields;
    if (fields.disk && !fields.disk.length) {
        fields.disk = "1/1";
    }
    const dir = file.containingDirectory.toLowerCase();
    if (dir.includes("bootleg")) {
        fields.comment = "bootleg";
    } else if (dir.includes("singles")) {
        fields.comment = "single";
    }
}

function changeTagFields(file, tag) {
    helpers.logTagInfo(file, tag);

    // change/add various fields; these values can still be overridden by specifying fields normally as CLI args
    applyFixesToMainFields(file, tag);
    clearPersonalFields(file, tag);
    addTotalNumberOfTracks(file, tag);
    addMiscFields(file, tag);
    if (helpers.isTruthy(settings.addLyrics)) {
        addLyrics(file, tag);
    }
    if (helpers.isTruthy(settings.addCover)) {
        addCover(file, tag);
    }
}
