//
// This is an example script demonstrating how the renaming tool can be used.
//

// script configuration

// specifies the separator between artist, album and track number
var separator = ", ";
// specifies the separator between title and other fields
var lastSeparator = " - ";
// specifies whether the artist name should be included
var includeArtist = false;
// specifies whether the album name should be included
var includeAlbum = false;
// specifies whether the title should be included
var includeTitle = true;
// specifies the distribution directory
// all files will be moved in an appropriate subdirectory in the
// distribution directory if one is specified
var distDir = false;
// string used for "miscellaneous" category
var misc = "misc";

// define helper functions

/*!
 * \brief Returns whether the specified \a value is not undefined
          and not an empty string.
 */
function notEmpty(value) {
    return value !== undefined && value !== "";
}
 
/*!
 * \brief Returns whether the specified \a value is not undefined
          and not zero.
 */
function notNull(value) {
    return value !== undefined && value !== 0;
}

/*!
 * \brief Returns the string representation of \a pos using at least as
          many digits as \a total has.
 */
function appropriateDigitCount(pos, total) {
    var res = pos + "";
    var count = (total + "").length;
    while (res.length < count) {
        res = "0" + res;
    }
    return res;
}

/*!
 * \brief Returns a copy of the specified \a name with characters that might be
          avoided in file names striped out.
 */
function validFileName(name) {
    if(name !== undefined) {
        return name.replace(/[\/\\<>?!*|:\"\n\f\r]/gi, "");
    } else {
        return "";
    }
}

/*!
 * \brief Returns a copy of the specified \a name with characters that might be
          avoided in directory names striped out.
 */
function validDirectoryName(name) {
    if(name !== undefined) {
        return name.replace(/[\/\\<>?!*|:\".\n\f\r]/gi, "");
    } else {
        return "";
    }
}

// the actual script

// check whether we have to deal with a file or a directory
if(isFile) {
    // parse file using the built-in parseFileInfo function
    var fileInfo = parseFileInfo(currentPath);
    var tag = fileInfo.tag; // get the tag information
    // read title and track number from the file name using the built-in parseFileName function
    var infoFromFileName = parseFileName(fileInfo.currentBaseName);
    // read the suffix from the file info object to filter backup and temporary files
    if(fileInfo.currentName === "desktop.ini") {
        action = actionType.skip; // skip these files
    } else if(fileInfo.currentSuffix === "bak") {
        // filter backup files by setting newRelativeDirectory to put them in a separate directory
        newRelativeDirectory = "backups";
    } else if(fileInfo.currentSuffix === "tmp") {
        // filter temporary files in the same way as backup files
        newRelativeDirectory = "temp";
    } else {
        // define an array for the fields; will be joined later
        var fields = [];
        // get the artist and remove invalid characters
        var artist = validFileName(tag.artist);
        // add artist to the fields array (if configured and present)
        if(includeArtist) {
            if(notEmpty(artist)) {
                fields.push(artist);
            } 
        }
        // get the album and remove invalid characters
        var album = validFileName(tag.album);
        // add album to the fields array (if configure and present)
        if(includeAlbum) {
            if(notEmpty(tag.album)) {
                fields.push(album);
            }  
        }
        // get the track/disk position; use the value from the tag if possible
        if(notNull(tag.trackPos)) {
            // define an array for the track position; will be joined later
            var pos = [];
            // push the disk position
            if(notNull(tag.diskPos)
                    && notNull(tag.diskTotal)
                    && tag.diskTotal >= 2) {
                pos.push(appropriateDigitCount(tag.diskPos, tag.diskTotal));
            }
            // push the track count
            if(notNull(tag.trackTotal)) {
                pos.push(appropriateDigitCount(tag.trackPos, tag.trackTotal));
            } else {
                pos.push(appropriateDigitCount(tag.trackPos, 10));
            }
            fields.push(pos.join("-"));
        } else if(notNull(infoFromFileName.trackPos)) {
            // get the track position from the file name if the tag has no track position field
            fields.push(appropriateDigitCount(infoFromFileName.trackPos, 10));
        }
        // join the first part of the new name
        newName = fields.join(separator);
        // get the title
        var title = validFileName(tag.title);
        // append the title (if configured and present)
        if(includeTitle) {
            // use value from file name if the tag has no title information
            if(!notEmpty(title)) {
                title = validFileName(infoFromFileName.title);
            }
            if(newName.length > 0) {
                newName = newName.concat(lastSeparator, title);
            } else {
                newName = newName.concat(title);
            }
        }
        // get an appropriate suffix
        var suffix = "";
        if(notEmpty(fileInfo.suitableSuffix)) {
            // get a suitable suffix from the file info object if available
            suffix = fileInfo.suitableSuffix;
        } else if(notEmpty(fileInfo.currentSuffix)) {
            // or just use the current suffix otherwise
            suffix = fileInfo.currentSuffix;
        }
        // append the suffix
        if(notEmpty(suffix)) {
            newName = newName.concat(".", suffix);
        }
        // set the distribution directory
        if(distDir) {
            var path = [distDir];
            var artist = validDirectoryName(tag.artist);
            if(notEmpty(artist)) {
                path.push(artist);
            } else {
                path.push(misc);
            }
            var album = validDirectoryName(tag.album);
            if(notEmpty(album)) {
                if(notEmpty(tag.year)) {
                    path.push([tag.year, album].join(" - "));
                } else {
                    path.push(album);
                }
            } else if(notEmpty(artist)) {
                path.push(misc);
            }
            if(tag.diskTotal >= 2) {
                path.push("Disk " + appropriateDigitCount(tag.diskPos, tag.diskTotal));
            }
            newRelativeDirectory = path.join("/");
        }
    }
    // set the action to "actionType.renaming"
    // (this is the default action, actually there is no need to set it explicitly)
    action = actionType.rename;
} else if(isDir) {
    // skip directories in this example script by setting the action to "actionType.skip"
    action = actionType.skip;
}
