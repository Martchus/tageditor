# Tag Editor
A tag editor with Qt GUI and command-line interface. Supports MP4 (iTunes), ID3, Vorbis, Opus, FLAC and Matroska.

## Supported formats
The tag editor can read and write the following tag formats:
- iTunes-style MP4/M4A tags (MP4-DASH is supported)
- ID3v1 and ID3v2 tags
  - conversion between ID3v1 and different versions of ID3v2 is possible
- Vorbis, Opus and FLAC comments in Ogg streams
  - cover art via "METADATA_BLOCK_PICTURE" is supported
- Vorbis comments and "METADATA_BLOCK_PICTURE" in raw FLAC streams
- Matroska/WebM tags and attachments

## Additional features
The tag editor can also display technical information such as the ID, format,
language, bitrate, duration, size, timestamps, sampling frequency, FPS and other information of the tracks.

It also allows to inspect and validate the element structure of MP4 and Matroska files.

## File layout options
### Tag position
The editor allows you to choose whether tags should be placed at the beginning or at
the end of an MP4/Matroska file.

In the CLI, this is controlled via `--tag-pos` option.
To enfore a specific `--tag-pos`, even if this requires the file to be rewritten, combine
with the `--force` option.

ID3v2 tags and Vorbis/Opus comments can only be placed at the beginning. ID3v1 tags
can only be placed at the end of the file.

### Index position
It is also possible to control the position of the index/cues. However, this is currently
only supported when dealing with Matroska files.

Note: This can not be implemented for MP4 since tags and index are tied to each other. When dealing
with MP4 files the index position will always be the same as the tag position.

#### Faststart
Putting the index at the beginning of the file is sometimes called *faststart*.

For forcing *faststart* via CLI the following options are required:

```
tageditor set --index-pos front --force
```

### Padding
Padding allows adding additional tag information without rewriting the entire file
or appending the tag. Usage of padding can be configured:
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
    * for binary repository checkout [my website](http://martchus.no-ip.biz/website/page.php?name=programming)
* Tumbleweed
    * for RPM \*.spec files and binary repository checkout
      [openSUSE Build Servide](https://build.opensuse.org/project/show/home:mkittler)
    * packages are available for x86_64, aarch64 and armv7l
    * since GCC provided by Leap is too old, only Tumbleweed packages are up-to-date
* Gentoo
    * packages are provided by perfect7gentleman; checkout his
      [repository](https://github.com/perfect7gentleman/pg_overlay)
* Windows
    * for mingw-w64 PKGBUILDs checkout [my GitHub repository](https://github.com/Martchus/PKGBUILDs)
    * for binaries checkout [my website](http://martchus.no-ip.biz/website/page.php?name=programming) and the
      release section on GitHub

## Usage
The Tag Editor has a GUI (Qt 5) and a command line interface. For a C++ library
interface checkout the underlying tagparser library.

### GUI
The GUI should be self-explaining. Just open a file, edit the tags and save the changings.

You can set the behaviour of the editor to keep previous values, so you don't have to enter
information like album name or artist for all files in an album again and again.

#### Screenshots
##### Main window under Openbox/qt5ct with Breeze theme/icons
![main window/Openbox/qt5ct/Breeze theme](/resources/screenshots/mainwindow.png?raw=true)

##### Main window under Plasma 5 with dark Breeze theme/icons
This screenshot shows the experimental MusicBrainz/LyricsWikia search.

![main window/Plasma 5/dark Breeze theme](/resources/screenshots/mainwindow-1366x768.png?raw=true)

#### Settings
Checkout the settings dialog. You can:
- customize which fields the editor shows and in which order
- change settings regarding the tag processing (ID3 version(s) to be used, preferred character set, usage of padding, ...)
- set whether unknown/unsupported tags should be ignored/kept or removed
- set whether ID3v1 and ID3v2 tags should be edited together or separately
- set the directory used to store temporary files
- set the desired file layout options (see section "File layout options")
- and more.

Settings of the GUI do not affect the CLI.

#### File renaming
There is also a tool to rename files using the tag information stored in the files. The new name is generated
by a small JavaScript which can be customized. An example script is provided. Before any actual changes are made,
you will see a preview with the generated file names.

#### MusicBrainz, Cover Art Archive and LyricaWiki search
The tag editor also features a MusicBrainz, Cover Art Archive and LyricaWiki search which can be opened with *F10*.
However, this feature is still experimental.

### CLI
#### Usage
```
tageditor <operation> [options]
```
Checkout the available operations and options with `--help`.

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


* Displays technical information about all \*.m4a files in the specified directory:
  ```
  tageditor info --files /some/dir/*.m4a
  ```

##### Writing tags
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

    - In particular, the custom field `FOO` is set to `bar1` in test.mkv and the custom field `©foo`
      is set to `bar2` in test.m4a. So the prefixes tell the tag editor that the specified field
      ID is a native field ID of a particular tag format rather than a generic identifier. Native
      fields are only applied to the corresponding format of course.
    - The following prefixes are supported:
        - `mp4`: iTune-style MP4/M4A ID (must be exactly 4 characters)
        - `mkv`: Matroska ID
        - `id3`: ID3v2 ID (must be exactly 3 or 4 characters depending on the tag version)
        - `vorbis`: Vorbis comment ID


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

### Building with Qt 5 GUI
The following Qt 5 modules are requried: core concurrent gui network widgets declarative/script webenginewidgets/webkitwidgets

#### Select Qt modules for JavaScript and WebView
* If Qt Script is installed on the system, the editor will link against it. Otherwise it will link against Qt QML.
* To force usage of Qt Script/Qt QML or to disable both add `-DJS_PROVIDER=script/qml/none` to the CMake arguments.
* If Qt WebKitWidgets is installed on the system, the editor will link against it. Otherwise it will link against Qt WebEngineWidgets.
* To force usage of Qt WebKit/Qt WebEngine or to disable both add `-DWEBVIEW_PROVIDER=webkit/webengine/none` to the CMake arguments.

### Building without Qt 5 GUI
It is possible to build without the GUI if only the CLI is needed. In this case no Qt dependencies (including qtutilities) are required.

To build without GUI, add the following parameters to the CMake call:
```
-DWIDGETS_GUI=OFF -DQUICK_GUI=OFF
```

### Building this straight
0. Install (preferably the latest version of) g++ or clang, the required Qt 5 modules and CMake.
1. Get the sources. For the lastest version from Git clone the following repositories:
   ```
   cd $SOURCES
   git clone https://github.com/Martchus/cpp-utilities.git c++utilities
   git clone https://github.com/Martchus/qtutilities.git
   git clone https://github.com/Martchus/tagparser.git
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

## TODO
- Support more formats (EXIF, PDF metadata, Theora in Ogg, ...)
- Allow adding tags to specific streams when dealing with Ogg
- Set tag information concurrently if multiple files have been specified (CLI)
- Support adding cue-sheet to FLAC files

## Bugs
- Large file information is not shown when using Qt WebEngine
- It is recommend to create backups before editing because I can not test whether the underlying library
  works with all kinds of files (when forcing rewrite a backup is always created).
- Also note the issue tracker on GitHub
