const lyricsCache = {};
const coverCache = {};
const albumColumn = 1;

export function queryLyrics(searchCriteria) {
    return cacheValue(lyricsCache, searchCriteria.title + "_" + searchCriteria.artist, () => {
        utility.log(" - Querying lyrics for '" + searchCriteria.title + "' from '" + searchCriteria.artist + "' ...");
        return queryLyricsFromProviders(["Tekstowo", "MakeItPersonal"], searchCriteria)
    });
}

export function queryCover(searchCriteria) {
    return cacheValue(coverCache, searchCriteria.album + "_" + searchCriteria.artist, () => {
        utility.log(" - Querying cover art for '" + searchCriteria.album + "' from '" + searchCriteria.artist + "' ...");
        return queryCoverFromProvider("MusicBrainz", searchCriteria);
    });
}

function cacheValue(cache, key, generator) {
    const cachedValue = cache[key];
    return cachedValue ? cachedValue : (cache[key] = generator());
}

function waitFor(signal) {
    signal.connect(() => { utility.exit(); });
    utility.exec();
}

function queryLyricsFromProvider(provider, searchCriteria) {
    const model = utility["query" + provider](searchCriteria);
    if (!model.areResultsAvailable) {
        waitFor(model.resultsAvailable);
    }
    if (!model.fetchLyrics(model.index(0, 0))) {
        waitFor(model.lyricsAvailable);
    }
    const lyrics = model.lyricsValue(model.index(0, 0));
    if (lyrics && lyrics.startsWith("Bots have beat this API")) {
        return undefined;
    }
    return lyrics;
}

function queryLyricsFromProviders(providers, searchCriteria) {
    for (const provider of providers) {
        const res = queryLyricsFromProvider(provider, searchCriteria);
        if (res) {
            return res;
        }
    }
}

function queryCoverFromProvider(provider, searchCriteria) {
    const context = searchCriteria.album + " from " + searchCriteria.artist;
    const model = utility["query" + provider](searchCriteria);
    if (!model.areResultsAvailable) {
        waitFor(model.resultsAvailable);
    }
    const albumUpper = searchCriteria.album.toUpperCase();
    utility.diag("debug", model.rowCount(), "rows");
    let row = 0, rowCount = model.rowCount();
    for (; row != rowCount; ++row) {
        const album = model.data(model.index(row, albumColumn));
        if (album && album.toUpperCase() === albumUpper) {
            break;
        }
    }
    if (row === rowCount) {
        utility.diag("debug", "unable to find meta-data on " + provider, context);
        return undefined;
    }
    if (!model.fetchCover(model.index(row, 0))) {
        waitFor(model.coverAvailable);
    }
    let cover = model.coverValue(model.index(row, 0));
    if (cover instanceof ArrayBuffer) {
        utility.diag("debug", "found cover", context);
        cover = utility.convertImage(cover, Qt.size(512, 512), "JPEG");
    }
    return cover;
}
