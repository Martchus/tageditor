// This is a simple example script demonstrating how the renaming tool can be used.
// script configuration

// skip directories in this example script
if (!tageditor.isFile) {
    tageditor.skip()
    return
}

// parse file using the built-in parseFileInfo function
const fileInfo = tageditor.parseFileInfo(tageditor.currentPath)
const tag = fileInfo.tag
const tracks = fileInfo.tracks

// deduce title and track number from the file name using the built-in parseFileName function (as fallback if tags missing)
const infoFromFileName = tageditor.parseFileName(fileInfo.currentBaseName)

// skip files which don't contain audio or video tracks
if (!fileInfo.hasAudioTracks && !fileInfo.hasVideoTracks) {
    tageditor.skip()
    return
}

// make new filename
const fieldsToInclude = [tag.albumartist || tag.artist, tag.album, tag.trackPos || infoFromFileName.trackPos, tag.title || infoFromFileName.title]
let newName = ""
for (let field of fieldsToInclude) {
    if (typeof field === "number" && tag.trackTotal) {
        field = field.toString().padStart(tag.trackTotal.toString().length, "0")
    }
    if (field && field.length !== 0) {
        newName = newName.concat(newName.length === 0 ? "" : " - ", field)
    }
}
newName = newName.concat(".", fileInfo.suitableSuffix || fileInfo.currentSuffix)

// apply new name
tageditor.rename(newName)
