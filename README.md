# Tag Editor
A tag editor with Qt GUI and command-line interface. Supports MP4 (iTunes), ID3, Vorbis, Opus, FLAC and Matroska.

## Supported formats
The tag editor can read and write the following tag formats:

* iTunes-style MP4/M4A tags (MP4-DASH is supported)
* ID3v1 and ID3v2 tags
    * conversion between ID3v1 and different versions of ID3v2 is possible
* Vorbis, Opus and FLAC comments in Ogg streams
    * cover art via "METADATA_BLOCK_PICTURE" is supported
* Vorbis comments and "METADATA_BLOCK_PICTURE" in raw FLAC streams
* Matroska/WebM tags and attachments

## Additional features
The tag editor can also display technical information such as the ID, format,
language, bitrate, duration, size, timestamps, sampling frequency, FPS and other information of the tracks.

It also allows to inspect and validate the element structure of MP4 and Matroska files.

## Backup/temporary files
Sometimes the tag editor has to rewrite the entire file in order to apply changes. With the default settings this
will happen most of the times.

The next section describes how to tweak settings to avoid this at the cost of having some padding within the files
and/or storing tags at the end of the file. **Note that temporary files also serve as a backup** in case something
goes wrong, e.g. your computer crashes while saving or a bug within the tag editor breaks particularly structured files.
To be safe it might therefore even be desirable to enforce the creation of a temporary file. That is also possible to
configure within the GUI and the CLI option `--force-rewrite`.

Nevertheless, it will not always be possible to avoid rewriting a file in all cases anyways. You can configure a
directory for temporary files within the GUI settings or the CLI option `--temp-dir`. Then you can easily clean up all
temporary files at some point together. For efficiency the temporary directory should be on the same file system
as the files you are editing. A feature to delete temporary files automatically has not been implemented yet.

## File layout options
### Tag position
The editor allows you to choose whether tags should be placed at the beginning or at the end of an MP4/Matroska file.
Placing tags at the end of the file can avoid having to rewrite the entire file to apply changes.

In the CLI, this is controlled via `--tag-pos` option.
To enforce a specific `--tag-pos`, even if this requires the file to be rewritten, combine with the `--force` option.

ID3v2 tags and Vorbis/Opus comments can only be placed at the beginning. ID3v1 tags can only be placed at the end of the
file. Hence, this configuration has no effect when dealing with such tags.

### Index position
It is also possible to control the position of the index/cues. However, this is currently only supported when dealing
with Matroska files.

Note: This can not be implemented for MP4 since tags and index are tied to each other. When dealing with MP4 files the
index position will always be the same as the tag position.

#### Faststart
Putting the index at the beginning of the file is sometimes called *faststart*.

For forcing *faststart* via CLI the following options are required:

```
tageditor set --index-pos front --force
```

### Padding
Padding allows adding additional tag information without rewriting the entire file or appending the tag. Usage of
padding can be configured:
- minimum/maximum padding: The file is rewritten if the padding would fall below/exceed the specified limits.
- preferred padding: If the file needs to be rewritten the preferred padding is used.

Default value for minimum and maximum padding is zero (in the CLI and GUI). This leads to the fact that
the Tag Editor will almost always have to rewrite the entire file to apply changes. To prevent this, set
at least the maximum padding to a higher value.

However, it is also possible to force rewriting the entire file.

The relevant CLI options are `--min-padding`, `--max-padding` and `--force-rewrite`.

Taking advantage of padding is currently not supported when dealing with Ogg streams (it is supported when
dealing with raw FLAC streams).

## Download
### Source
See the release section on GitHub.

### Packages and binaries
* Arch Linux
    * for PKGBUILDs checkout [my GitHub repository](https://github.com/Martchus/PKGBUILDs) or
      [the AUR](https://aur.archlinux.org/packages?SeB=m&K=Martchus)
    * there is also a [binary repository](https://martchus.no-ip.biz/repo/arch/ownstuff)
* Tumbleweed, Leap, Fedora
    * RPM \*.spec files and binaries are available via openSUSE Build Service
        * remarks
            * Be sure to add the repository that matches the version of your OS and to keep it
              in sync when upgrading.
            * The linked download pages might be incomplete, use the repositories URL for a full
              list.
        * latest releases: [download page](https://software.opensuse.org/download.html?project=home:mkittler&package=tageditor),
          [repositories URL](https://download.opensuse.org/repositories/home:/mkittler),
          [project page](https://build.opensuse.org/project/show/home:mkittler)
        * Git master: [download page](https://software.opensuse.org/download.html?project=home:mkittler:vcs&package=tageditor),
          [repositories URL](https://download.opensuse.org/repositories/home:/mkittler:/vcs),
          [project page](https://build.opensuse.org/project/show/home:mkittler:vcs)
* Exherbo
    * there is a [package in the platypus repository](https://git.exherbo.org/summer/packages/media-sound/tageditor)
* Gentoo
    * there is a [package in perfect7gentleman's repository](https://github.com/perfect7gentleman/pg_overlay)
* Other GNU/Linux systems
    * [AppImage repository for releases on the openSUSE Build Service](https://download.opensuse.org/repositories/home:/mkittler:/appimage/AppImage)
    * [AppImage repository for builds from Git master the openSUSE Build Service](https://download.opensuse.org/repositories/home:/mkittler:/appimage:/vcs/AppImage/)
* Windows
    * for mingw-w64 PKGBUILDs checkout [my GitHub repository](https://github.com/Martchus/PKGBUILDs)
    * for binaries checkout the [release section on GitHub](https://github.com/Martchus/tageditor/releases)
        * the Qt 6 based version is stable and preferable but only supports Windows 10
        * the Qt 5 based version should still work on older versions down to Windows 7 although this is not regularly checked

## Usage
The Tag Editor has a Qt-based GUI and a command line interface. For a C++ library
interface checkout the underlying tagparser library.

### GUI
The GUI should be self-explaining - a lot of the UI elements have tooltips with further explanations.
The basic workflow is quite simple:

0. Open a file
1. Edit the tags
2. Save changes

You can set the behaviour of the editor to keep previous values, so you don't have to enter
information like album name or artist for all files in an album again and again.

Note that the GUI does *not* support setting multiple values of the same field (besides covers of
different types). If a file already contains fields with multiple values, the additional values
are discarded. Use the CLI if support for multiple values per field is required but note that not
all tag formats support this anyways.

#### Screenshots
##### Main window under Openbox/qt5ct with Breeze theme/icons
![main window/Openbox/qt5ct/Breeze theme](/resources/screenshots/mainwindow.png?raw=true)

##### Main window under Plasma 5 with dark Breeze theme/icons
This screenshot shows the experimental MusicBrainz/LyricWiki search.

![main window/Plasma 5/dark Breeze theme](/resources/screenshots/mainwindow-1366x768.png?raw=true)

#### Settings
Checkout the settings dialog. You can
- customize which fields the editor shows and in which order.
- change settings regarding the tag processing (ID3 version(s) to be used, preferred character set, usage of padding, ...).
- set whether unknown/unsupported tags should be ignored/kept or removed.
- set whether ID3v1 and ID3v2 tags should be edited together or separately.
- set the directory used to store temporary files.
- set the desired file layout options (see section "File layout options").
- enable auto-correction features like trimming whitespaces.

Settings of the GUI do not affect the CLI.

#### File renaming
There is also a tool to rename files using the tag information stored in the files. The new name for each file is
generated by a small JavaScript which can be customized. An example script is provided. Before any actual changes are
made, you will see a preview with the generated file names. As shown in the example script it is also possible to
move files into another directory.

#### MusicBrainz, Cover Art Archive and LyricWiki search
The tag editor also features a MusicBrainz, Cover Art Archive and LyricWiki search.

0. Open the search by pressing *F10*.
1. Open the first file in the album.
2. If album and artist are already present those are automatically inserted as search terms. Otherwise you need to
   enter the album and artist to search for manually. It makes most sense to search for an entire album and therefore
   leave the fields for track number and title blank.
3. Click on "Search" and select on of the query options.
4. Select the row and fields to be used and click on "Use row". You can also double click the row you want to use.
5. Use the "Insert automatically" option to insert search results for the next files automatically when you open them.
   To find the matching row for the song in the search results the title or track number needs to be present. If artist
   and album are present as well these are also tried to match for further disambiguation.

##### Notes
* The context menu on the list of results provides further options, e.g. to open the result in a web browser.
* There are shortcuts to trigger the different queries.
* LyricWiki was shut down completely on September 21, 2020 so the LyricWiki search is no longer working.

### CLI
#### Usage
```
tageditor <operation> [options]
```
Checkout the available operations and options with `--help`. For a list of all available field names, track
attribute names and modifier, use the CLI option `--print-field-names`.

#### Examples
Here are some Bash examples which illustrate getting and setting tag information:

##### Reading tags
* Displays title, album and artist of all \*.m4a files in the specified directory:  
  ```
  tageditor get title album artist --files /some/dir/*.m4a
  ```

* Displays all supported fields of all \*.mkv files in the specified directory:  
  ```
  tageditor get --files /some/dir/*.mkv
  ```

* Extracts the cover of the specified (Opus) file:  
  ```
  tageditor extract cover --output-file the-cover.jpg --file some-file.opus
  ```

    - No conversion is done by the tag editor. This command assumes that the cover is a JPEG image.
    - The extraction works for other fields like lyrics as well.
    - For Matroska attachments one needs to use `--attachment`.

* Displays technical information about all \*.m4a files in the specified directory:  
  ```
  tageditor info --files /some/dir/*.m4a
  ```

##### Modifying tags and track attributes
* Sets title, album, artist, cover and track number of all \*.m4a files in the specified directory:  
  ```
  tageditor set title="Title of "{1st,2nd,3rd}" file" title="Title of "{4..16}"th file" \
    album="The Album" artist="The Artist" \
    cover'=/path/to/image' lyrics'>=/path/to/lyrics' track'+=1/16' --files /some/dir/*.m4a
  ```

    - As shown, it is possible to specify multiple files at once.
    - The `>` after the field name `lyrics` causes the tag editor to read the value from the specified file.
      This works for other fields as well and is implied for `cover`.
    - The `+` sign after the field name `track` indicates that the field value should be increased after a file
      has been processed.
    - Specifying a field multiple times allows to specify different values for the different files.
        - So in this example, the first file will get the title *Title of 1st file*, the second file will get the
          name *Title of 2nd file* and so on.
        - The 16th and following files will all get the title *Title of the 16th file*.
        - All files will get the album name *The Album*, the artist *The Artist* and the cover image from the
          file `/path/to/image` and the lyrics from the file `/path/to/lyrics`.

* Sets title of both specified files and the album of the second specified file:  
  ```
  tageditor set title0="Title for both files" album1="Album for 2nd file" \
    --files file1.ogg file2.mp3
  ```
  The number after the field name specifies the index of the first file to use the value for. The first index is 0.

* Sets the title specifically for the track with the ID ``3134325680`` and removes
  the tags targeting the song/track and the album/movie/episode in general:  
  ```
  tageditor set target-level=30 target-tracks=3134325680 title="Title for track 3134325680" \
    --remove-target target-level=50 --remove-target target-level=30 \
    --files file.mka
  ```
  For more information checkout the [Matroska specification](https://matroska.org/technical/specs/tagging/index.html).

* Sets custom fields:  
  ```
  tageditor set mkv:FOO=bar1 mp4:©foo=bar2 -f file.mkv file.m4a
  ```

    - In particular, the custom field `FOO` is set to `bar1` in file.mkv and the custom field `©foo`
      is set to `bar2` in file.m4a. So the prefixes tell the tag editor that the specified field
      ID is a native field ID of a particular tag format rather than a generic identifier. Native
      fields are only applied to the corresponding format of course.
    - The following prefixes are supported:
        - `mp4`: iTune-style MP4/M4A ID (must be exactly 4 characters)
        - `mkv`: Matroska ID
        - `id3`: ID3v2 ID (must be exactly 3 or 4 characters depending on the tag version)
        - `vorbis`: Vorbis comment ID, also works for Opus (which uses Vorbis comments as well)

* Removes the "forced" flag from all tracks, flags the track with the ID 2 as "default" and sets its language to "ger":  
  ```
  tageditor set track-id=all forced=no track-id=2 default=yes language=ger
  ```

    - So modifying track attributes is possible as well and it works like setting tag fields.
    - Specific tracks can currently only be referred by ID.

* Here is another example, demonstrating the use of arrays and the syntax to auto-increase numeric fields such as the track number:  
  ```
  cd some/dir
  # create an empty array
  titles=()
  # iterate through all music files in the directory
  for file in *.m4a; do \
      # truncate the first 10 characters
      title="${file:10}"; \
      # append the title truncating the extension
      titles+=("title=${title%.*}"); \
  done
  # now set the titles and other tag information
  tageditor set "${titles[@]}" album="Some Album" track+=1/25 disk=1/1 -f *.m4a
  ```  

* Sets a cover of a special type with a description:  
  ```
  tageditor set cover=":front-cover" cover0="/path/to/back-cover.jpg:back-cover:The description" -f foo.mp3
  ```

    - The general syntax is `path:cover-type:description`. The cover type and description are optional.
    - The `0` after the 2nd `cover` is required. Otherwise the 2nd cover would only be set in the 2nd file (which
      is not even specified in this example).
    - In this example the front cover is removed (by passing an empty path) and the back cover set to the specified
      file and description. Existing covers of other types and with other descriptions are not affected.
    - When specifying a cover without type, all existing covers are replaced and the new cover will be of the
      type "other".
        - To replace all existing covers when specifying a cover type
          use e.g. `… cover= cover0="/path/to/back-cover.jpg:back-cover"`.
    - It is possible to add multiple covers of the same type with only different descriptions. However, when
      leaving the description out (2nd `:` is absent), all existing covers of the specified type are replaced and
      the new cover will have an empty description.
        - To affect only the covers with an empty description, be sure to keep the trailing `:`.
        - To remove all existing covers of a certain type but regardless of the description
          use e.g. `… cover=":back-cover"`.
    - The names of all cover types can be shown via `tageditor --print-field-names`.
    - This is only supported by the tag formats ID3v2 and Vorbis Comment. The type and description are ignored
      when dealing with a different format.

## Text encoding / unicode support
1. It is possible to set the preferred encoding used *within* the tags via CLI option ``--encoding``
   and in the GUI settings.
   * However, in the GUI this only affects visible fields. In the CLI only specified fields are
     affected. So reencoding all text fields is currently not supported.
   * The option is ignored (with a warning) when the specified encoding is not supported by the
     tag format.
2. The CLI assumes all arguments to be UTF-8 encoded (no matter which preferred encoding is specified)
   and file names are just passed as specified.
3. The CLI prints all values in UTF-8 encoding (no matter which encoding is actually used in the tag).

## Build instructions
The application depends on [c++utilities](https://github.com/Martchus/cpp-utilities) and
[tagparser](https://github.com/Martchus/tagparser) and is built the same way as these libraries.
For basic instructions checkout the README file of [c++utilities](https://github.com/Martchus/cpp-utilities).
When the Qt GUI is enabled, Qt and [qtutilities](https://github.com/Martchus/qtutilities) are required, too.

To avoid building c++utilities/tagparser/qtutilities separately, follow the instructions under
"Building this straight". There's also documentation about
[various build variables](https://github.com/Martchus/cpp-utilities/blob/master/doc/buildvariables.md) which
can be passed to CMake to influence the build.

### Building with Qt GUI
The Qt GUI is enabled by default. The following Qt modules are required (version 5.6 or higher):
core concurrent gui network widgets declarative/script webenginewidgets/webkitwidgets

Note that old Qt versions like 5.6 lack support for modern JavaScript features. To use the JavaScript-based
renaming tool it is recommend to use at least Qt 5.12.

#### Select Qt module for web view and JavaScript
* Add `-DWEBVIEW_PROVIDER:STRING=webkit/webengine/none` to the CMake arguments to use either Qt WebKit (works with
  'revived' version as well), Qt WebEngine or no web view at all. If no web view is used, the file information can only
  be shown using a plain tree view. Otherwise the user can choose between a web page and a tree view.
* Add `-DJS_PROVIDER:STRING=script/qml/none` to the CMake arguments to use either Qt Script, Qt QML or no JavaScript
  engine at all. If no JavaScript engine is used, the renaming utility is disabled.

### Building without Qt GUI
It is possible to build without the GUI if only the CLI is needed. In this case no Qt dependencies (including qtutilities) are required.

To build without GUI, add the following parameters to the CMake call:
```
-DWIDGETS_GUI=OFF -DQUICK_GUI=OFF
```

### JSON export
As a small demo for [Reflective RapidJSON](https://github.com/Martchus/reflective-rapidjson), the tag editor features an optional
JSON export. To enable it, add `-DENABLE_JSON_EXPORT` to the CMake arguments.

When enabled, the following additional dependencies are required (only at build-time): rapidjson, reflective-rapidjson and llvm/clang

### Building this straight
0. Install (preferably the latest version of) the CGG toolchain or Clang, the required Qt modules,
   [iso-codes](https://salsa.debian.org/iso-codes-team/iso-codes), iconv, zlib, CMake and Ninja.
1. Get the sources of additional dependencies and the tag editor itself.
   For the latest version from Git clone the following repositories:
   ```
   cd "$SOURCES"
   git clone -c core.symlinks=true https://github.com/Martchus/cpp-utilities.git c++utilities
   git clone -c core.symlinks=true https://github.com/Martchus/tagparser.git
   git clone -c core.symlinks=true https://github.com/Martchus/qtutilities.git                  # only required for Qt GUI
   git clone -c core.symlinks=true https://github.com/Martchus/reflective-rapidjson.git         # only required for JSON export
   git clone -c core.symlinks=true https://github.com/Martchus/tageditor.git
   git clone -c core.symlinks=true https://github.com/Martchus/subdirs.git
   ```
   Note that `-c core.symlinks=true` is only required under Windows to handle symlinks correctly.
   This requires a recent Git version and a filesystem which supports symlinks (NTFS works).
   If you run into "not found" errors on symlink creation use `git reset --hard` within the repository to
   fix this.
2. Configure the build
   ```
   cd "$BUILD_DIR"
   cmake \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DLANGUAGE_FILE_ISO_639_2="/usr/share/iso-codes/json/iso_639-2.json" \
    -DCMAKE_INSTALL_PREFIX="/install/prefix" \
    "$SOURCES/subdirs/tageditor"
   ```
    * Replace `/install/prefix` with the directory where you want to install.
    * Replace `/usr/…/iso_639-2.json` with the path for your iso-codes installation.
3. Build and install everything in one step:
   ```
   cd "$BUILD_DIR"
   ninja install
   ```
    * If the install directory is not writable, do **not** conduct the build as root. Instead, set `DESTDIR` to a
      writable location (e.g. `DESTDIR="temporary/install/dir" ninja install`) and move the files from there to
      the desired location afterwards.

## TODOs and known problems
* Support more formats (JPEG/EXIF, PDF metadata, Theora in Ogg, ...)
* Allow adding tags to specific streams when dealing with Ogg
* Set tag information concurrently if multiple files have been specified (CLI)
* Support adding cue-sheet to FLAC files
* Generally improve the CLI to make its use more convenient and cover more use-cases

### Known problems and caveats
* Large file information is not shown when using Qt WebEngine or the GUI becomes unresponsive. Use the feature to parse the full
  file structure in combination with the Qt WebEngine-based view with caution.
* It is recommend to create backups before editing because I can not test whether the underlying library
  works with all kinds of files (when forcing rewrite a backup is always created).
* More TODOs and bugs are tracked in the [issue section at GitHub](https://github.com/Martchus/tageditor/issues).

### Windows-specific issues
The following caveats apply to Windows' default terminal emulator `cmd.exe`. I recommend to use Mintty (e.g. via MSYS2) instead.

* The console's codepage is set to UTF-8 to ensure point *3.* of the "Text encoding" section is handled correctly. Use
  `set ENABLE_CP_UTF8=0` if this is not wanted.
* To enable console output for Tag Editor which is built as a GUI application it is attaching to the parent
  processes' console. However, this prevents redirections to work. If redirections are needed, use
  `set ENABLE_CONSOLE=0` to disable that behavior.

---

The dark mode introduced with Windows 10 is not supported but this can be
[worked around](https://github.com/Martchus/syncthingtray#workaround-missing-support-for-windows-10-dark-mode).

---

Per monitor DPI awareness (v2) is not working out of the box but experimental support
[can be enabled](https://github.com/Martchus/syncthingtray#enable-experimental-support-for-windows-per-monitor-dpi-awareness-v2).

## License
All code is licensed under [GPL-2-or-later](LICENSE).
