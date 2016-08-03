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

ID3v2 tags and Vorbis/Opus comments can only be placed at the beginning. ID3v1 tags
can only be placed at the end of the file.

### Padding
Padding allows adding additional tag information without rewriting the entire file
or appending the tag. Usage of padding can be configured:
- minimum/maximum padding: The file is rewritten if the padding would fall below/exceed the specifed limits.
- preferred padding: If the file needs to be rewritten the preferred padding is used.

However, it is also possible to force rewriting the entire file.

Taking advantage of padding is currently not supported when dealing with Ogg streams (it is supported when
dealing with raw FLAC streams).

## Download / binary repository
I currently provide packages for Arch Linux and Windows. For more information checkout my
[website](http://martchus.no-ip.biz/website/page.php?name=programming).

## Usage
The Tag Editor has a GUI (Qt 5) and a command line interface. For a C++ library
interface checkout the underlying tagparser library.

### GUI
The GUI should be self-explaining. Just open a file, edit the tags and save the changings.

You can set the behaviour of the editor to keep previous values, so you don't have to enter
information like album name or artist for all files in an album again and again.

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

#### MusicBrainz search
The tag editor also features a MusicBrainz and Cover Art Archive search which can be opened with *F10*. However, this feature is still experimental.

### CLI
#### Usage
```
tageditor <operation> [options]
```
Checkout the available operations and options with --help.

#### Examples
Here are some Bash examples which illustrate getting and setting tag information:

##### Reading tags
* *Displays* title, album and artist of all *.m4a files in the specified directory:
  ```
  tageditor get title album artist --files /some/dir/*.m4a
  ```

* *Displays* all supported fields of all *.mkv files in the specified directory:
    ```
    tageditor get --files /some/dir/*.mkv
    ```


  * *Displays* technical information about all *.m4a files in the specified directory:
    ```
    tageditor info --files /some/dir/*.m4a
    ```

* *Displays* technical information about all *.m4a files in the specified directory:
  ```
  tageditor info --files /some/dir/*.m4a
  ```

**Note**: All values are printed in UTF-8 encoding, no matter which  encoding is actually used within the tag.

##### Writing tags
* *Sets* title, album, artist, cover and track number of all *.m4a files in the specified directory:

  ```
  tageditor set title="Title of "{1st,2nd,3rd}" file" title="Title of "{4..16}"th file" \
    album="The Album" artist="The Artist" \
    cover=/path/to/image track={1..16}/16 --files /some/dir/*.m4a
  ```

  - The first file will get the title *Title of 1st file*, the second file will get the name *Title of 2nd file* and so on.
  - The 16th and following files will all get the title *Title of the 16th file*.
  - The same scheme is used for the track numbers.
  - All files will get the album name *The Album*, the artist *The Artist* and the cover image from the file */path/to/image*.

  **Note:** The current version v1.4.0 has a bug so tagging multiple files at once doesn't work as shown above. As a workaround
  use either the Git version or use title0=... title1=... title2=... to specify the different titles for the files.

* *Sets* title of both specified files and the album of the second specified file:
    ```
    tageditor set title0="Title for both files" album1="Album for 2nd file" \
      --files file1.ogg file2.mp3
    ```
    The number after the field name specifies the index of the first file to use the value for. The first index is 0.

* *Sets* the title specificly for the track with the ID ``3134325680`` and removes
  the tags targeting the song/track and the album/movie/episode in general:
    ```
    tageditor set target-level=30 target-tracks=3134325680 title="Title for track 3134325680" \
      --remove-targets target-level=50 , target-level=30 \
      --files file.mka
    ```
    For more information checkout the [Matroska specification](https://matroska.org/technical/specs/tagging/index.html).

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

**Note**: All specified values are assumed to be UTF-8 encoded, no matter which encoding has been specified as preferred encoding via ``--encoding`` option. (This mentioned option only affects the encoding to be used *within* the tag.)

## Build instructions
The application depends on [c++utilities](https://github.com/Martchus/cpp-utilities) and [tagparser](https://github.com/Martchus/tagparser) and is built the same way as these libaries. For basic instructions checkout the README file of [c++utilities](https://github.com/Martchus/cpp-utilities).

### Building with Qt 5 GUI
The following Qt 5 modules are requried: core concurrent gui network declarative/script widgets webenginewidgets/webkitwidgets

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

## TODO
- Support more formats (EXIF, PDF metadata, Theora in Ogg, ...).
- Allow adding tags to specific streams when dealing with Ogg.
- Do tests with Matroska files which have multiple segments.
- Set tag information concurrently if multiple files have been specified (CLI).

## Bugs
- Large file information is not shown when using Qt WebEngine.
- It is recommend you to create backups before editing because I can not test whether the
  library works with all kind of files. If you force rewriting a backup is always created.
- underlying library: Matroska files composed of more than one segment aren't tested yet and might not work.
