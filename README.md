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
