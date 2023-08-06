const cache = {};

function waitFor(signal) {
    signal.connect(() => { utility.exit(); });
    utility.exec();
}

function queryProvider(provider, searchCriteria) {
    const lyricsModel = utility["query" + provider](searchCriteria);
    if (!lyricsModel.areResultsAvailable) {
        waitFor(lyricsModel.resultsAvailable);
    }
    if (!lyricsModel.fetchLyrics(lyricsModel.index(0, 0))) {
        waitFor(lyricsModel.lyricsAvailable);
    }
    const lyrics = lyricsModel.lyricsValue(lyricsModel.index(0, 0));
    if (lyrics && lyrics.startsWith("Bots have beat this API")) {
        return undefined;
    }
    return lyrics;
}

function queryProviders(providers, searchCriteria) {
    for (const provider of providers) {
        const res = queryProvider(provider, searchCriteria);
        if (res) {
            return res;
        }
    }
}

export function queryLyrics(searchCriteria) {
    const cacheKey = searchCriteria.title + "_" + searchCriteria.artist;
    const cachedValue = cache[cacheKey];
    return cachedValue
        ? cachedValue
        : cache[cacheKey] = queryProviders(["Tekstowo", "MakeItPersonal"], searchCriteria);
}


