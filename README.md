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
To be save it might therefore even be desirable to enforce the creation of a temporary file. That is also possible to
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
To enfore a specific `--tag-pos`, even if this requires the file to be rewritten, combine with the `--force` option.

ID3v2 tags and Vorbis/Opus comments can only be placed at the beginning. ID3v1 tags can only be placed at the end of the
file. Hence this configuration has no effect when dealing with such tags.

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
- minimum/maximum padding: The file is rewritten if the padding would fall below/exceed the specifed limits.
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
        * latest releases: [download page](https://software.opensuse.org/download.html?project=home:mkittler&package=tageditor),
          [project page](https://build.opensuse.org/project/show/home:mkittler)
        * Git master: [download page](https://software.opensuse.org/download.html?project=home:mkittler:vcs&package=tageditor),
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
    cover=/path/to/image track={1..16}/16 --files /some/dir/*.m4a
  ```

    - The first file will get the title *Title of 1st file*, the second file will get the name *Title of 2nd file* and so on.
    - The 16th and following files will all get the title *Title of the 16th file*.
    - The same scheme is used for the track numbers.
    - All files will get the album name *The Album*, the artist *The Artist* and the cover image from the file */path/to/image*.

* Sets title of both specified files and the album of the second specified file:  
  ```
  tageditor set title0="Title for both files" album1="Album for 2nd file" \
    --files file1.ogg file2.mp3
  ```
  The number after the field name specifies the index of the first file to use the value for. The first index is 0.

* Sets the title specificly for the track with the ID ``3134325680`` and removes
  the tags targeting the song/track and the album/movie/episode in general:  
  ```
  tageditor set target-level=30 target-tracks=3134325680 title="Title for track 3134325680" \
    --remove-targets target-level=50 , target-level=30 \
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
  **Note**: The *+* sign after the field name *track* which indicates that the field value should be increased after
  a file has been processed.

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

### Windows only
  * Codepage is set to UTF-8 to ensure *3.* is handled correctly by the terminal. So
    far it seems that this effort just causes weird truncating behaviour in some cases and might
    prevent pipes/redirections to function. One can use MSYS2 terminal to work around this.
  * All UTF-16 encoded arguments as provided by WinAPI are converted to UTF-8 so *2.*
    shouldn't cause any trouble.
  * A workaround to support filenames containing non-ASCII characters (despite the lack of an UTF-8
    supporting `std::fstream` under Windows) can be enabled by adding `-DUSE_NATIVE_FILE_BUFFER=ON`
    to the CMake arguments **when building `c++utilities`**. It is *not* sufficient to specify this
    option only when building `tagparser` or Tag Editor.


## Build instructions
The application depends on [c++utilities](https://github.com/Martchus/cpp-utilities) and [tagparser](https://github.com/Martchus/tagparser) and is built the same way as these libaries. For basic instructions checkout the README file of [c++utilities](https://github.com/Martchus/cpp-utilities).  
When the Qt GUI is enabled, Qt and [qtutilities](https://github.com/Martchus/qtutilities) are required, too.

### Building with Qt GUI
The following Qt modules are requried (version 5.6 or higher): core concurrent gui network widgets declarative/script webenginewidgets/webkitwidgets

### Select Qt module for web view and JavaScript
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
0. Install (preferably the latest version of) g++ or clang, the required Qt modules and CMake.
1. Get the sources of additional dependencies and the tag editor itself. For the lastest version from Git clone the following repositories:  
   ```
   cd $SOURCES
   git clone https://github.com/Martchus/cpp-utilities.git c++utilities
   git clone https://github.com/Martchus/tagparser.git
   git clone https://github.com/Martchus/qtutilities.git                  # only required for Qt GUI
   git clone https://github.com/Martchus/reflective-rapidjson.git         # only required for JSON export
   git clone https://github.com/Martchus/tageditor.git
   git clone https://github.com/Martchus/subdirs.git
   ```
2. Build and install everything in one step:  
   ```
   cd $BUILD_DIR
   cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="/install/prefix" \
    $SOURCES/subdirs/tageditor
   make install -j$(nproc)
   ```

## TODOs
* Support more formats (JPEG/EXIF, PDF metadata, Theora in Ogg, ...)
* Allow adding tags to specific streams when dealing with Ogg
* Set tag information concurrently if multiple files have been specified (CLI)
* Support adding cue-sheet to FLAC files

### Bugs
* Large file information is not shown when using Qt WebEngine
* It is recommend to create backups before editing because I can not test whether the underlying library
  works with all kinds of files (when forcing rewrite a backup is always created).
* Also note the issue tracker on GitHub

More TODOs and bugs are tracked in the [issue section at GitHub](https://github.com/Martchus/tageditor/issues).
