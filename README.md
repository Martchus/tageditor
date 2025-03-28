# Tag Editor
A tag editor with Qt GUI and command-line interface. Supports MP4 (iTunes), ID3, Vorbis, Opus, FLAC and Matroska.

## Supported formats
The tag editor can read and write the following tag formats:

* iTunes-style MP4/M4A tags (MP4-DASH is supported)
* ID3v1 and ID3v2 tags
    * conversion between ID3v1 and different versions of ID3v2 is possible
    * mainly for use in MP3 files but can be added to any kind of file
* Vorbis, Opus and FLAC comments in Ogg streams
    * cover art via "METADATA_BLOCK_PICTURE" is supported
* Vorbis comments and "METADATA_BLOCK_PICTURE" in raw FLAC streams
* Matroska/WebM tags and attachments

Further remarks:

* Unsupported file contents (such as unsupported tag formats) are *generally* preserved as-is.
* Note that APE tags are *not* supported. APE tags in the beginning of a file are strongly
  unrecommended and thus discarded when applying changes. APE tags at the end of the file
  are preserved as-is when applying changes.

## Additional features
The tag editor can also display technical information such as the ID, format,
language, bitrate, duration, size, timestamps, sampling frequency, FPS and other information of the tracks.

It also allows to inspect and validate the element structure of MP4 and Matroska files.

## Backup/temporary files
Sometimes the tag editor has to rewrite the entire file in order to apply changes. This leads to the creation
of a temporary file. With the GUI's default settings this is even enforced to be conservative **as the temporary files
also serve as a backup** in case something goes wrong, e.g. your computer crashes while saving or a bug within the tag
editor breaks particularly structured files. When using the CLI it is therefore also recommend to use `--force-rewrite`.

The next section describes how to tweak settings to avoid rewriting at the cost of having *no* backup, having some
padding within the files and/or storing tags at the end of the file.

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
- preferred padding: If the file is rewritten the preferred padding is used.

It is also possible to force rewriting the entire file to enforce the preferred padding is used.

The relevant CLI options are `--min-padding`, `--max-padding`, `--preferred-padding` and `--force-rewrite`.

Taking advantage of padding is currently not supported when dealing with Ogg streams (it is supported when
dealing with raw FLAC streams).

### Avoid rewriting files
As explained in the "[Backup/temporary files](#backuptemporary-files)" section, this is not a good idea as the
temporary file that is created when rewriting the entire file also serves as backup. However, if you nevertheless
want to avoid rewriting the file as much as possible, set the following in the GUI's "File layout" settings:

* "Force rewrite…" option: unchecked
* "Use preferred position…" options: unchecked
* Minimum padding: 0
* Maximum padding: 429496729 (simply a very high number)

When using the CLI, you just need to add `--max-padding 429496729` to the CLI arguments (and avoid any of the other
arguments mentioned in previous sections).

### Improve performance
Editing big files (especially Matroska files) can take some time. To improve the performance, put the index at the
end of the file (CLI option `--index-pos back`) because then the size of the index will never have to be recalculated.
Also follow the advice from the "[Backup/temporary files](#backuptemporary-files)" section to force rewriting and to
put the temporary directory on the same filesystem as the file you are editing. Forcing a rewrite can improve the
performance because then the tag editor will not even try to see whether it could be avoided and can thus skip
computations that can take a notable time for big Matroska files.

Of course being able to avoid a rewrite would still be more optimal. Checkout the previous section for how to achieve
that. To improve performance further when avoiding a rewrite, put the tag at the end (CLI option `--tag-pos back`).
Then the tag editor will not even try to put tags at the front and can thus skip a few computations. (Avoiding a
rewrite is still not a good idea in general.)

## Matroska-related remarks
The Matroska container format (and WebM which is based on Matroska) are breaking with common conventions. Therefore
not all CLI examples mentioned below make sense to use on such files.

Generally, one Matroska file can have multiple tags and each tag has a "target" which decides to what the fields of
the tag apply to, e.g. the song or the whole album. So when using the CLI or the GUI you need to be aware to what
tag/target to add fields to.

Matroska also does *not* use one combined field for the track/disk number and total like other formats do. It instead
uses the separate fields `part` and `totalparts` which again need to be added to a tag of the desired target (e.g.
50/"ALBUM" for the track number and total).

Checkout the official [Matroska documentation on tagging](https://matroska.org/technical/tagging.html) for details.
It also contains examples for [audio content](https://matroska.org/technical/tagging-audio-example.html) and
[video content](https://matroska.org/technical/tagging-video-example.html).

Note that Tag Editor does *not* support the XML format mentioned on the Matroska documentation. In the GUI you can
simply add/remove/edit tags and their targets via the controls at the top of the editor. In the settings you can also
specify that tags of certain targets should be added automatically when loading a file. When using the CLI you can
specify that a field should be added to a tag of a certain target by specifying the target *before* that field. You
can also explicitly remove tags of certain targets. Examples of the concrete CLI usage can be found below.

## Download
### Source
See the [release section on GitHub](https://github.com/Martchus/tageditor/releases).

### Packages and binaries
* Arch Linux
    * for PKGBUILDs checkout [my GitHub repository](https://github.com/Martchus/PKGBUILDs) or
      [the AUR](https://aur.archlinux.org/packages?SeB=m&K=Martchus)
    * there is also a [binary repository](https://martchus.dyn.f3l.de/repo/arch/ownstuff)
* Tumbleweed, Leap, Fedora
    * RPM \*.spec files and binaries are available via openSUSE Build Service
        * remarks
            * Install preferably the `tageditor-qt6` package if available for your OS.
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
    * there is a [package in perfect7gentleman's repository](https://gitlab.com/Perfect_Gentleman/PG_Overlay)
* Void Linux
    * available as package `tageditor` from the
      [official repositories](https://voidlinux.org/packages/?q=tageditor)
* Other GNU/Linux systems
    * for generic, self-contained binaries checkout the [release section on GitHub](https://github.com/Martchus/tageditor/releases)
        * Requires glibc>=2.26, OpenGL and libX11
            * openSUSE Leap 15, Fedora 27, Debian 10 and Ubuntu 18.04 are recent enough (be sure
              the package `libopengl0` is installed on Debian/Ubuntu)
        * Supports X11 and Wayland (set the environment variable `QT_QPA_PLATFORM=xcb` to disable
          native Wayland support if it does not work on your system)
        * Binaries are signed with the GPG key
          [`B9E36A7275FC61B464B67907E06FE8F53CDC6A4C`](https://keyserver.ubuntu.com/pks/lookup?search=B9E36A7275FC61B464B67907E06FE8F53CDC6A4C&fingerprint=on&op=index).
* Windows
    * for binaries checkout the [release section on GitHub](https://github.com/Martchus/tageditor/releases)
        * Windows SmartScreen will likely block the execution (you'll get a window saying "Windows protected your PC");
          right click on the executable, select properties and tick the checkbox to allow the execution
        * Antivirus software often **wrongly** considers the executable harmful. This is a known problem. Please don't create
          issues about it.
        * The Qt 6 based version is stable and preferable but only supports Windows 10 version 1809 and newer.
        * The Qt 5 based version should still work on older versions down to Windows 7 although this is not regularly checked.
        * The Universal CRT needs to be [installed](https://learn.microsoft.com/en-us/cpp/windows/universal-crt-deployment#central-deployment).
        * Binaries are signed with the GPG key
          [`B9E36A7275FC61B464B67907E06FE8F53CDC6A4C`](https://keyserver.ubuntu.com/pks/lookup?search=B9E36A7275FC61B464B67907E06FE8F53CDC6A4C&fingerprint=on&op=index).
    * there is also a [Chocolatey package](https://community.chocolatey.org/packages/tageditor) maintained by bcurran3
    * for mingw-w64 PKGBUILDs checkout [my GitHub repository](https://github.com/Martchus/PKGBUILDs)

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

#### Limitations
The GUI does *not* support setting multiple values of the same field (besides covers of
different types). If a file already contains fields with multiple values, the additional values
are discarded. Use the CLI if support for multiple values per field is required. Not all tag formats
support this anyways, though.

The GUI does *not* support batch processing. I recommend using the CLI for this.

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
- set the desired file layout options (see section "[File layout options](#file-layout-options)").
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
attribute names and modifier, use the CLI option `--print-field-names`. Not all fields are supported by all
tag/container formats. Most notably, the Matroska container format does not use `track`/`disk` to store the
track/disk number and total in one field. Instead, the fields `part` and `totalparts` need to be used on the
desired target.

Note that Windows users must use `tageditor-cli.exe` instead of `tageditor.exe` or use Mintty as terminal.
Checkout the "[Windows-specific issues](#windows-specific-issues)" section for details.

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

##### Modifying tags and track attributes<a name="writing-tags"></a>
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

    - The general syntax is `path:cover-type:description`. The cover type and description are optional. The
      delimiter can be changed via `--cover-type-delimiter` which is useful if the path includes a `:`.
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

* Sets fields by running a script to compute changes dynamically:
  ```
  tageditor set --pedantic debug --script path/to/script.js -f foo.mp3
  ```

    - This feature is still experimental. The script API is still subject to change.
    - The script needs to be ECMAScript as supported by the Qt framework.
    - This feature requires the tag editor to be configured with Qt QML as JavaScript provider at
      compile time. Checkout the build instructions under "Building with Qt GUI" for details.
    - The script needs to export a `main()` function. This function is invoked for every file and
      passed an object representing the current file as first argument. The file only modified if
      `main()` returns a truthy value or `undefined`; otherwise the file is skipped completely (and
      thus not modified at all, so values passed via `--values` are not applied).
    - Checkout the file
      [`resources/scripts/scriptapi/resize-covers.js`](resources/scripts/scriptapi/resize-covers.js)
      in this repository that scales all cover images in a file down to the specified maximum size.
      It can be invoked like this:
      ```
      tageditor set --pedantic debug --script :/scripts/resize-covers.js --script-settings coverSize=512 coverFormat=JPEG` -f …
      ```
      Note that this example uses a path starting with `:/scripts/…` to use the built-in version of
      that script. In case you want to modify the script you can of course grab it from this code
      repository, do the editing and then just specify a normal path.
    - Checkout the file
      [`resources/scripts/scriptapi/set-tags.js`](resources/scripts/scriptapi/set-tags.js) in this
      repository for an example that applies basic fixes and tries to fetch lyrics and cover art
      when according settings are passed. It can be invoked like this:
      ```
      tageditor set --pedantic debug --script :/scripts/set-tags.js --script-settings addCover=1 addLyrics=1
      ```
      The remark about the path from the previous point applies here as well.
    - For debugging, the option `--pedantic debug` is very useful. You may also add
      `--script-settings dryRun=1` and check for that setting within the script as shown in
      `resources/scripts/scriptapi/resize-covers.js` and `resources/scripts/scriptapi/set-tags.js`.
    - Common tag fields are exposed as object properties as shown in the mentioned example files.
        - Only properties for fields that are supported by the tag are added to the "fields" object.
        - Adding properties of unsupported fields manually does not work; those will just be ignored.
        - The properties for fields that are absent in the tag have an empty array assigned. You may
          also assign an empty array to fields to delete them.
        - The content of binary fields is exposed as `ArrayBuffer`. Use must also use an `ArrayBuffer`
          to set the value of binary fields such as the cover.
        - The content of other fields is mostly exposed as `String` or `Number`. You must also use
          one of those types to set the value of those fields. The string-representation of the
          assigned content will then be converted automatically to what's needed internally.
    - The `utility` object exposes useful methods, e.g. for logging and controlling the event loop.
    - Checkout the file `resources/scripts/scriptapi/http.js` in this repository for an example of
      using XHR and controlling the event loop.
    - The script runs after tags are added/removed (according to options like `--id3v1-usage`).
      So the tags present during script execution don't necessarily represent tags that are actually
      present in the file on disk (but rather the tags that will be present after saving the file).
    - The script is executed before any other modifications are applied. So if you also specify
      values as usual (via `--values`) then these values have precedence over values set by the
      script.
    - It is also possible to rename the file (via e.g. `file.rename(newPath)`). This will be done
      immediately and also if `main()` returns a falsy value (so it is possible to only rename a
      file without modifying it by returning a falsy value). If the specified path is relative, it
      is interpreted relative to current directory of the file (and *not* to the current working
      directory of the tag editor).
    - It is also possible to open another file via `utility.openFile(path)`. This makes it possible
      to copy tags over from another file, e.g. to insert tags back from original files that have
      been lost when converting to a different format. The mentioned example script `set-tags.js`
      also demonstrates this for covers and lyrics when according script settings are passed (e.g.
      `--script-settings addCover=1 originalDir=… originalExt=…`). Example:
      ```
      cd '/…/music'
      find 'artist/album' -iname '*.m4a' -exec tageditor set \
        --pedantic debug \
        --script :/scripts/set-tags.js \
        --script-settings dryRun=1 originalDir='../music-lossless' originalExt=.flac addCover=1 addLyrics=1 \
        --temp-dir /…/tmp/tageditor \
        --files {} \+
      ```

##### Further useful commands
* Let the tag editor return with a non-zero exit code even if only non-fatal problems have been encountered
    * when saving a file:  
      ```
      tageditor set ... --pedantic warning -f ...
      ```  
    * when printing technical information to validate the structure of a file:  
      ```
      tageditor info  --pedantic warning --validate -f ...
      ```  
        - This is especially useful for MP4 and Matroska files where the tag editor will be able to emit
          warnings and critical messages when those files are truncated or have a broken index.

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
The Qt GUI is enabled by default. The following Qt modules are required (only the latest Qt 5 and Qt 6 version
tested):
core concurrent gui network widgets declarative/script webenginewidgets/webkitwidgets

Note that old Qt versions lack support for modern JavaScript features. To use the JavaScript-based renaming tool
it is recommend to use at least Qt 5.12.

To specify the major Qt version to use, set `QT_PACKAGE_PREFIX` (e.g. add `-DQT_PACKAGE_PREFIX:STRING=Qt6`
to the CMake arguments).

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
JSON export. To enable it, add `-DENABLE_JSON_EXPORT=ON` to the CMake arguments.

When enabled, the following additional dependencies are required (only at build-time): rapidjson, reflective-rapidjson and llvm/clang

### Building this straight
0. Install (preferably the latest version of) the GCC toolchain or Clang, the required Qt modules,
   [iso-codes](https://salsa.debian.org/iso-codes-team/iso-codes), iconv, zlib, CMake and Ninja.
1. Get the sources of additional dependencies and the tag editor itself.
   For the latest version from Git clone the following repositories:
   ```
   cd "$SOURCES"
   git config core.symlinks true                                          # only required on Windows
   git clone https://github.com/Martchus/cpp-utilities.git c++utilities
   git clone https://github.com/Martchus/tagparser.git
   git clone https://github.com/Martchus/qtutilities.git                  # only required for Qt GUI
   git clone https://github.com/Martchus/reflective-rapidjson.git         # only required for JSON export
   git clone https://github.com/Martchus/tageditor.git
   git clone https://github.com/Martchus/subdirs.git
   ```
   Note that `git config core.symlinks=true` is only required under Windows to handle symlinks correctly.
   This requires a recent Git version and a filesystem which supports symlinks (NTFS works). Additionally,
   you need to
   [enable Windows Developer Mode](https://learn.microsoft.com/en-us/gaming/game-bar/guide/developer-mode).
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
The following caveats can be worked around by using the CLI-wrapper instead of the main executable. This is the
file that ends with `-cli.exe`. Alternatively you may use Mintty (e.g. via MSYS2) which is also not affected by
those issues:

* The console's codepage is set to UTF-8 to ensure point *3.* of the "[Text encoding](#text-encoding--unicode-support)"
  section is handled correctly. This may not work well under older Windows versions. Use `set ENABLE_CP_UTF8=0` if this
  is not wanted.
* The main application is built as a GUI application. To nevertheless enable console output it is attaching to the
  parent processes' console. However, this prevents redirections to work in most cases. If redirections are needed,
  use `set ENABLE_CONSOLE=0` to disable that behavior.

---

Dark mode should work out of the box under Windows 11 and can otherwise be enabled by
[selecting the Fusion style](https://github.com/Martchus/syncthingtray#tweak-gui-settings-for-dark-mode-under-windows).

---

Tag Editor supports
[PMv2](https://docs.microsoft.com/en-us/windows/win32/hidpi/high-dpi-desktop-application-development-on-windows#per-monitor-and-per-monitor-v2-dpi-awareness)
out of the box as of Qt 6. You may tweak settings according to the
[Qt documentation](https://doc.qt.io/qt-6/highdpi.html#configuring-windows).

## Copyright notice and license
Copyright © 2015-2025 Marius Kittler

All code is licensed under [GPL-2-or-later](LICENSE).

## Attribution for 3rd party content
* The program icon is based on icons taken from the [KDE/Breeze](https://invent.kde.org/frameworks/breeze-icons) project.
* Other icons found in this repository are taken from the [KDE/Breeze](https://invent.kde.org/frameworks/breeze-icons) project
  (without modification).
