 # MusicBrainz API
* Regular meta data is available at ```http://musicbrainz.org/ws/2``` and ```http://mb.videolan.org/ws/2```.
* Cover art is available at ```http://coverartarchive.org```.

## Most useful queries
* artist by name
  ```
   /artist/?query=artist:$artist_name&inc=releases
  ```
* albums of artist
  ```
   /artist/$artist_id?inc=releases
  ```
* songs of album
  ```
   /release/$album_id?inc=recordings
  ```
* song by name, album and artist
  ```
   /recording?query="$song_name" AND artist:"$artist_name" AND release:"$release_name"
  ```
* album by name
  ```
  /release?query="$album_name"
  ```
* cover art by album ID
   ```
   /release/$album_id/front-500
   ```
* lyrics: TODO


## Misc
* enable JSON response: ```&fmt=json```

## See also
* [Web Service](https://wiki.musicbrainz.org/Development/JSON_Web_Service)
* [Web Service/Search](https://wiki.musicbrainz.org/Development/XML_Web_Service/Version_2/Search)

# Other sources for meta data
TODO
