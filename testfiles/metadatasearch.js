import * as http from "http.js"

function waitFor(signal) {
    signal.connect(() => { utility.exit(); });
    utility.exec();
}

function queryMakeItPersonal(searchCriteria) {
    const lyricsModel = utility.queryMakeItPersonal(searchCriteria);
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

export function queryLyrics(searchCriteria) {
    return queryMakeItPersonal(searchCriteria);
}


