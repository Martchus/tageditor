// This is an example script demonstrating how the renaming tool can be used.
// script configuration

// specifies the separator between artist, album and track number
var separator = ", "
// specifies the separator between title and other fields
var lastSeparator = " - "
// specifies whether the artist name should be included
var includeArtist = true
// specifies whether the album name should be included
var includeAlbum = true
// specifies whether the title should be included
var includeTitle = true
// specifies whether tags like [foo] should be stripped from the title if deduced from file name
var stripTags = false
// specifies whether the title deduced from the file name should be kept as-is
var keepTitleFromFileName = false
// specifies whether track information should be appended (like [H.265-320p AAC-LC-2ch-eng AAC-LC-2ch-ger])
var includeTrackInfo = false
// specifies the "distribution directory"
//var distDir = false                        // don't move files around
var distDir = "/path/to/my/music-collection" // move files to an appropriate subdirectory under this path
// directory used to store collections which contain songs from multiple artists
var collectionsDir = "collections"
// directory used to store miscellaneous songs by miscellaneous artists
var miscDir = "misc"
// directory used for miscellaneous songs by specific artist
var miscAlbumDir = "misc"
// condition to move files to miscDir
var isMiscFile = function (tag) {
    return tag.comment === "misc"
}
// condition to consider files part of a collection which contains songs from multiple artists
var isPartOfCollection = function (tag) {
    return tag.comment === "collection"
}

//
// helper functions
//

// returns whether the specified \a value is not undefined and not an empty string.
function notEmpty(value) {
    return value !== undefined && value !== ""
}
// returns whether the specified \a value is not undefined and not zero.
function notNull(value) {
    return value !== undefined && value !== 0
}
// returns the string representation of \a pos using at least as many digits as \a total has
function appropriateDigitCount(pos, total) {
    var res = pos + ""
    var count = (total + "").length
    while (res.length < count) {
        res = "0" + res
    }
    return res
}
// returns a copy of the specified \a name with characters that might be avoided in file names striped out
function validFileName(name) {
    return name !== undefined ? name.replace(/[\/\\]/gi, " - ").replace(
        /[<>?!*|:\"\n\f\r]/gi, "") : ""
}
// returns a copy of the specified \a name with characters that might be avoided in directory names striped out.
function validDirectoryName(name) {
    return name !== undefined ? name.replace(/[\/\\]/gi, " - ").replace(
        /[<>?!*|:\".\n\f\r]/gi, "") : ""
}
// strips tags from the beginning or end of the string if configured
function tagsStripped(name) {
    return stripTags ? name.replace(/^(\[[^\]]*\]\s*)+/g, "").replace(/(\s*\[[^\]]*\])+$/g, "") : name
}

//
// actual script
//

// skip directories in this example script
if (!tageditor.isFile) {
    tageditor.skip()
    return
}

// parse file using the built-in parseFileInfo function
var fileInfo = tageditor.parseFileInfo(tageditor.currentPath)
var tag = fileInfo.tag
var tracks = fileInfo.tracks

// deduce title and track number from the file name using the built-in parseFileName function (as fallback if tags missing)
var infoFromFileName = tageditor.parseFileName(fileInfo.currentBaseName)

// skip hidden and "desktop.ini" files
if (fileInfo.currentName.indexOf(".") === 0 ||
    fileInfo.currentName === "desktop.ini") {
    tageditor.skip()
    return
}

// treat *.lrc files like their corresponding audio files
var keepSuffix
if (fileInfo.currentSuffix === "lrc") {
    keepSuffix = fileInfo.currentSuffix // keep the lrc suffix later
    for (let extension of ["mp3", "flac", "m4a"]) {
        fileInfo = tageditor.parseFileInfo(fileInfo.currentPathWithoutExtension + "." + extension)
        tag = fileInfo.tag
        if (!fileInfo.ioErrorOccured) {
            break
        }
    }
    if (fileInfo.ioErrorOccured) {
        tageditor.skip("skipping, corresponding audio file not present")
        return
    }
}

// skip files which don't contain audio or video tracks
if (!fileInfo.hasAudioTracks && !fileInfo.hasVideoTracks) {
    tageditor.skip()
    return
}

// filter backup and temporary files by putting them in a separate directory
if (fileInfo.currentSuffix === "bak") {
    tageditor.move("backups")
    return
}
if (fileInfo.currentSuffix === "tmp") {
    tageditor.move("temp")
    return
}

// define an array for the fields to be joined later
var fields = []

// get the artist (preferably album artist), remove invalid characters and add it to fields array
var artist = validFileName(tag.albumartist || tag.artist)
if (includeArtist && !isPartOfCollection(tag) && notEmpty(artist)) {
    fields.push(artist)
}

// get the album and remove invalid characters and add it to fields array
var album = validFileName(tag.album)
if (includeAlbum && notEmpty(tag.album)) {
    fields.push(album)
}

// get the track/disk position and add it to fields array
// use the value from the tag if possible; otherwise the value deduced from the filename
if (notNull(tag.trackPos)) {
    var pos = []
    // push the disk position
    if (notNull(tag.diskPos) && notNull(tag.diskTotal) && tag.diskTotal >= 2) {
        pos.push(appropriateDigitCount(tag.diskPos, tag.diskTotal))
    }
    // push the track count
    if (notNull(tag.trackTotal)) {
        pos.push(appropriateDigitCount(tag.trackPos, tag.trackTotal))
    } else {
        pos.push(appropriateDigitCount(tag.trackPos, 10))
    }
    fields.push(pos.join("-"))
} else if (notNull(infoFromFileName.trackPos)) {
    fields.push(appropriateDigitCount(infoFromFileName.trackPos, 10))
}

// join the first part of the new name
var newName = fields.join(separator)

// get the title and append it
var title = validFileName(tag.title)
if (includeTitle) {
    // use value from file name if the tag has no title information
    if (!notEmpty(title)) {
        if (!keepTitleFromFileName) {
            title = validFileName(tagsStripped(infoFromFileName.title))
        }
        if (!notEmpty(title)) {
            title = validFileName(tagsStripped(fileInfo.currentBaseName))
        }
    }
    if (newName.length > 0) {
        newName = newName.concat(lastSeparator, title)
    } else {
        newName = newName.concat(title)
    }
}

// append track info
if (includeTrackInfo && tracks.length > 0) {
    newName = newName.concat(" [", tracks.map(track => track.description).join(" "), "]")
}

// append an appropriate suffix
var suffix = ""
if (notEmpty(fileInfo.suitableSuffix)) {
    // get a suitable suffix from the file info object if available
    suffix = fileInfo.suitableSuffix
} else if (notEmpty(fileInfo.currentSuffix)) {
    // or just use the current suffix otherwise
    suffix = fileInfo.currentSuffix
}
if (notEmpty(suffix)) {
    newName = newName.concat(".", suffix)
}

// apply new name
tageditor.rename(newName)

// set the distribution directory
if (!distDir) {
    return
}
var path = [distDir]
var artist = validDirectoryName(tag.albumartist || tag.artist)
if (isPartOfCollection(tag)) {
    path.push(collectionsDir)
} else if (isMiscFile(tag)) {
    path.push(miscDir)
} else if (notEmpty(artist)) {
    path.push(artist)
} else {
    path.push(miscDir)
}
var album = validDirectoryName(tag.album)
if (notEmpty(album)) {
    if (notEmpty(tag.year)) {
        path.push([tag.year.split("-")[0], album].join(" - "))
    } else {
        path.push(album)
    }
} else if (notEmpty(artist) && !isMiscFile(tag)) {
    path.push(miscAlbumDir)
}
if (tag.diskTotal >= 2) {
    path.push("Disk " + appropriateDigitCount(tag.diskPos, tag.diskTotal))
}
// apply new relative directory
tageditor.move(path.join("/"))