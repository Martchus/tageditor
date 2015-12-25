# Tag Editor
A tag editor with Qt GUI and command-line interface. Supports MP4 (iTunes), ID3, Vorbis and Matroska.

## Supported formats
The tag editor can read and write the following tag formats:
- iTunes-style MP4 tags (MP4-DASH is supported)
- ID3v1 and ID3v2 tags
- Vorbis comments (cover art via "METADATA_BLOCK_PICTURE" is supported)
- Matroska/WebM tags and attachments

## Additional features
The tag editor can also display technical information such as the ID, format, language, bitrate,
duration, size, timestamps, sampling frequency, FPS and other information of the tracks.

It also allows to inspect and validate the element structure of MP4 and Matroska files.

## File layout options
### Tag position
The editor allows you to choose whether tags should be placed at the beginning or at
the end of an MP4/Matroska file.

### Padding
Padding allows adding additional tag information without rewriting the entire file
or appending the tag. Usage of padding can be configured:
- minimum/maximum padding: The file is rewritten if the padding would fall below/exceed the specifed limits.
- preferred padding: If the file needs to be rewritten the preferred padding is used.

However, it is also possible to force rewriting the entire file.

## Download / repository
I currently provide packages for Arch Linux and Windows. For more information checkout my
[website](http://martchus.netai.net/page.php?name=programming).

## Usage
The Tag Editor has a GUI (Qt 5) and a command line interface.

### GUI
The GUI should be self-explaining. Just open a file, edit the tags and save the changings.

You can set the behaviour of the editor to keep previous values, so you don't have to enter
information like album name or artist for all files in an album again and again.

#### Settings
Checkout the settings dialog. You can:
- customize which fields the editor shows and in which order
- change settings regarding the tag processing (ID3 version(s) to be used, preferred character set,
  usage of padding, ...)
- set whether unknown/unsupported tags should be ignored/kept or removed
- set whether ID3v1 and ID3v2 tags should be edited together or separately
- set the directory used to store temporary files
- and more.

Settings of the GUI do not affect the CLI.

#### File renaming
There is also a tool to rename files using the tag information stored in the files. The new name is generated
by a small JavaScript which can be customized. An example script is provided. Before any actual changes are made,
you will see a preview with the generated file names.

### CLI
Usage:
```
tageditor <operation> [options]
```

Checkout the available operations and options with --help.
Here are some Bash examples which illustrate getting and setting tag information:

* Displays title, album and artist of all *.m4a files in the specified directory:

  ```
  tageditor get title album artist --files /some/dir/*.m4a
  ```
  
* Displays technical information about all *.m4a files in the specified directory:

  ```
  tageditor info --files /some/dir/*.m4a
  ```

* Sets title, album, artist, cover and track number of all *.m4a files in the specified directory:

  ```
  tageditor set "title=Title of "{1st,2nd,3rd}" file" "title=Title of "{4..16}"th file" \
    "album=The Album" "artist=The Artist" \
    cover=/path/to/image track={1..16}/16 --files /some/dir/*.m4a
  ```
  
  The first file will get the name *Title of 1st file*, the second file will get the name *Title of 2nd file* and so on.
  The 16th and following files will all get the name *Title of the 16th file*. The same scheme is used for the track numbers.
  All files will get the album name *The Album*, the artist *The Artist* and the cover image from the file */path/to/image*.

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
  tageditor set "${titles[@]}" "album=Some Album" track+=1/25 disk=1/1 -f *.m4a
  ```
  
  Note the *+* sign after the field name *track* which indicates that the field value should be increased after
  a file has been processed.

## Build instructions
The application depends on c++utilities, qtutilities and tagparser and is built in the same way as these libaries.

The following Qt 5 modules are requried: core gui script widgets webenginewidgets/webkitwidgets

If webkitwidgets is installed on the system, the editor will link against it. Otherwise it will link against webenginewidgets. To force usage of webkitwidgets
add "CONFIG+=forcewebkit" to the qmake arguments.

## TODO
- Support more tag formats (EXIF, PDF metadata, ...).
- Set tag information concurrently if multiple files have been specified (CLI).
- Do tests with Matroska files which have multiple segments.

## Bugs
- Large file information is not shown when using Qt WebEngine.
- Matroska files composed of more than one segment aren't tested yet and might not work.
