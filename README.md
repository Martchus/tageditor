# tageditor
A tageditor with Qt GUI and command line interface. Supports MP4 (iTunes), ID3, Vorbis and Matroska.

## Supported formats
The tag editor can read and write the following tag formats:
- iTunes-style MP4 tags (MP4-DASH is supported)
- ID3v1 and ID3v2 tags
- Vorbis comments
- Matroska/WebM tags and attachments

The tag editor can also display technical information such as the ID, format, bitrate,
duration and timestamps of the tracks.
It also allows to inspect and validate the element structure of MP4 and Matroska files.

## Build instructions
The tageditor depends on c++utilities, qtutilities and tagparser. Is built in the same way as these libaries.

The following Qt 5 modules are requried: core gui script widgets webkitwidgets

## Usage

### GUI
The GUI should be self-explaining. Just open a file, edit the tags and save the changings.

You can set the behaviour of the editor to keep previous values, so you don't have to enter
information like album name or artist for all files in an album again and again.

Checkout the settings dialog. You can customize which fields the editor shows,
change some settings regarding the tags processing (ID3 version, preferred character set, ...) and more.

There is also a tool to rename files using the tag information stored in the files. The new name is generated
by a small JavaScript which can be customized. An example script is provided. Before any changes are made,
you can checkout a preview with the generated file names.

### CLI
Checkout the available command line options with --help. Here are Bash examples which illustrate
getting and setting tag information:

```
tageditor get title album artist --files /some/dir/*.m4a
```
Displays title, album and artist of all *.m4a files in the specified directory.

```
tageditor set "title=Title of "{1st,2nd,3rd}" file" "title=Title of "{1st,2nd,3rd}" file" "title=Title of "{4..16}"th file" "album=The Album" "artist=The Artist" cover=/path/to/image track={1..16}/16 --files /some/dir/*.m4a
```
Sets title, album, artist, cover and track number of all *.m4a files in the specified directory.
The first file will get the name "Title of 1st file", the second file will get the name "Title of 2nd file" and so on.
The 16th and following files will all get the name "Title of the 16th file". The same scheme is used for the track numbers.
All files will get the album name "The Album", the artist "The Artist" and the cover image from the file "/path/to/image".
