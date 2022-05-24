// This is an example script demonstrating how the renaming tool can be used.
// script configuration

// specifies the separator between artist, album and track number
const separator = ", "
// specifies the separator between title and other fields
const lastSeparator = " - "
// specifies whether the artist name should be included
const includeArtist = true
// specifies whether the album name should be included
const includeAlbum = true
// specifies whether the title should be included
const includeTitle = true
// specifies whether tags like [foo] should be stripped from the title if deduced from file name
const stripTags = false
// specifies whether the title deduced from the file name should be kept as-is
const keepTitleFromFileName = false
// specifies whether track information should be appended (like [H.265-320p AAC-LC-2ch-eng AAC-LC-2ch-ger])
const includeTrackInfo = false
// specifies the "distribution directory"
const distDir = false                            // don't move files around
//const distDir = "/path/to/my/music-collection" // move files to an appropriate subdirectory under this path
// move tracks into subdirectories for ranges (like 0001, 0025, 0050, 0100, …) to avoid too many files within one level
const maxTracksPerDir = false
//var maxTracksPerDir = 25
// directory used to store collections which contain songs from multiple artists
const collectionsDir = "collections"
// directory used to store miscellaneous songs by miscellaneous artists
const miscDir = "misc"
// directory used for miscellaneous songs by specific artist
const miscAlbumDir = "misc"
// condition to move files to miscDir
const isMiscFile = tag => tag.comment === "misc"
// condition to move files to miscAlbumDir
const isMiscAlbumFile = tag => tag.comment === "miscalbum"
// condition to consider files part of a collection which contains songs from multiple artists
const isPartOfCollection = tag => tag.comment === "collection"

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
    return pos.toString().padStart(total.toString().length, "0")
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
// strips trailing brackets
function trailingBracketsStripped(name) {
    return name.replace(/ \(.*\)/gi, '')
}
// returns the first value of a semi-colon separated list
function firstValue(list) {
    return list !== undefined ? list.split(";")[0] : undefined
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
    fields.push(trailingBracketsStripped(firstValue(artist)))
}

// get the album and remove invalid characters and add it to fields array
var album = validFileName(tag.album)
if (includeAlbum && notEmpty(tag.album)) {
    fields.push(trailingBracketsStripped(album))
}

// get the track/disk position and add it to fields array
// use the value from the tag if possible; otherwise use the value deduced from the filename
var trackPos = tag.trackPos || infoFromFileName.trackPos
var pos = []
// push the disk position
if (notNull(tag.diskPos) && (!notNull(tag.diskTotal) || tag.diskTotal >= 2)) {
    pos.push(appropriateDigitCount(tag.diskPos, tag.diskTotal || 1))
}
// push the track count
if (notNull(trackPos)) {
    pos.push(appropriateDigitCount(tag.trackPos, tag.trackTotal || 10))
}
if (pos.length) {
    fields.push(pos.join("-"))
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
var path = []
if (distDir) {
    path.push(distDir)
    var artist = validDirectoryName(firstValue(tag.albumartist || tag.artist))
    if (isPartOfCollection(tag)) {
        path.push(collectionsDir)
    } else if (notEmpty(artist) && !isMiscFile(tag)) {
        path.push(artist)
    } else {
        path.push(miscDir)
    }
    var album = validDirectoryName(tag.album)
    if (notEmpty(album) && !isMiscAlbumFile(tag)) {
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
}

// set "range directory"
if (!trackPos && (trackPos = fileInfo.currentBaseName.match(/\d+/))) {
    trackPos = parseInt(trackPos) // if there's no track pos, take first best number from filename
}
if (maxTracksPerDir && notNull(trackPos)) {
    path.push(((Math.floor(trackPos / maxTracksPerDir) * maxTracksPerDir) || 1).toString().padStart(4, "0"))
}

// apply new relative directory
if (path.length) {
    tageditor.move(path.join("/"))
}
