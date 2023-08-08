#include "./tekstowo.h"

#include "../application/settings.h"
#include "../misc/networkaccessmanager.h"
#include "../misc/utility.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QTextDocumentFragment>
#include <QUrl>

#include <functional>

using namespace std;
using namespace std::placeholders;
using namespace Utility;

namespace QtGui {

static const auto defaultTekstowoUrl = QStringLiteral("https://www.tekstowo.pl");

static QUrl tekstowoUrl()
{
    const auto &url = Settings::values().dbQuery.tekstowoUrl;
    return QUrl(url.isEmpty() ? defaultTekstowoUrl : url);
}

TekstowoResultsModel::TekstowoResultsModel(SongDescription &&initialSongDescription, QNetworkReply *reply)
    : HttpResultsModel(std::move(initialSongDescription), reply)
{
}

bool TekstowoResultsModel::fetchLyrics(const QModelIndex &index)
{
    if ((index.parent().isValid() || !index.isValid() || index.row() >= m_results.size()) || !m_results[index.row()].lyrics.isEmpty()) {
        return true;
    }
    const auto url = webUrl(index);
    if (url.isEmpty()) {
        m_errorList << tr("Unable to fetch lyrics: web URL is unknown.");
        emit resultsAvailable();
        return true;
    }
    auto *reply = Utility::networkAccessManager().get(QNetworkRequest(url));
    addReply(reply, bind(&TekstowoResultsModel::handleLyricsReplyFinished, this, reply, index.row()));
    return false;
}

void TekstowoResultsModel::parseInitialResults(const QByteArray &data)
{
    beginResetModel();
    m_results.clear();
    auto dropLast = false;
    auto hasExactMatch = false;
    auto exactMatch = QList<SongDescription>::size_type();
    for (auto index = exactMatch; index >= 0;) {
        const auto linkStart = data.indexOf("<a href=\"/piosenka,", index);
        if (linkStart < 0) {
            break;
        }
        const auto hrefStart = linkStart + 9;
        const auto hrefEnd = data.indexOf("\"", hrefStart + 1);
        if (hrefEnd <= hrefStart) {
            break;
        }
        const auto linkEnd = data.indexOf("</a>", hrefEnd);
        if (linkEnd < linkStart) {
            break;
        }
        index = linkEnd + 4;
        auto linkText = QTextDocumentFragment::fromHtml(QString::fromUtf8(data.begin() + linkStart, linkEnd + 3 - linkStart)).toPlainText().trimmed();
        auto titleStart = linkText.indexOf(QLatin1String(" - "));
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        auto &songDetails = dropLast ? m_results.back() : m_results.emplace_back();
#else
        if (!dropLast) {
            m_results.append(SongDescription());
        }
        auto &songDetails = m_results.back();
#endif
        songDetails.songId = QTextDocumentFragment::fromHtml(QString::fromUtf8(data.begin() + hrefStart, hrefEnd - hrefStart)).toPlainText();
        if (titleStart > -1) {
            songDetails.artist = linkText.mid(0, titleStart);
            if (songDetails.artist != m_initialDescription.artist) {
                dropLast = true;
                continue;
            }
            songDetails.title = linkText.mid(titleStart + 3);
        } else {
            songDetails.title = std::move(linkText);
        }
        if (!hasExactMatch && songDetails.title == m_initialDescription.title) {
            hasExactMatch = true;
            exactMatch = m_results.size() - 1;
        }
        dropLast = false;
    }
    if (dropLast) {
        m_results.pop_back();
    }
    // ensure the first exact match for the song title is placed first
    if (hasExactMatch && exactMatch != 0) {
        std::swap(m_results[exactMatch], m_results[0]);
    }
    endResetModel();
}

void TekstowoResultsModel::handleLyricsReplyFinished(QNetworkReply *reply, int row)
{
    auto data = QByteArray();
    if (auto *newReply = evaluateReplyResults(reply, data, true)) {
        addReply(newReply, bind(&TekstowoResultsModel::handleLyricsReplyFinished, this, newReply, row));
        return;
    }
    if (!data.isEmpty()) {
        parseLyricsResults(row, data);
    }
    if (!m_resultsAvailable) {
        setResultsAvailable(true);
    }
}

void TekstowoResultsModel::parseLyricsResults(int row, const QByteArray &data)
{
    if (row >= m_results.size()) {
        m_errorList << tr("Internal error: context for Tekstowo page reply invalid");
        setResultsAvailable(true);
        return;
    }
    auto lyricsStart = data.indexOf("<div class=\"inner-text\">");
    if (lyricsStart < 0) {
        const auto &assocDesc = m_results[row];
        m_errorList << tr("Song details requested for %1/%2 do not contain lyrics").arg(assocDesc.artist, assocDesc.title);
        setResultsAvailable(true);
        return;
    }
    const auto lyricsEnd = data.indexOf("</div>", lyricsStart += 24); // hopefully lyrics don't contain nested </div>
    m_results[row].lyrics = QTextDocumentFragment::fromHtml(
        QString::fromUtf8(data.data() + lyricsStart, lyricsEnd > -1 ? lyricsEnd - lyricsStart : data.size() - lyricsStart))
                                .toPlainText()
                                .trimmed();
    setResultsAvailable(true);
    emit lyricsAvailable(index(row, 0));
}

QUrl TekstowoResultsModel::webUrl(const QModelIndex &index)
{
    if (index.parent().isValid() || !index.isValid() || index.row() >= m_results.size()) {
        return QUrl();
    }
    auto url = tekstowoUrl();
    url.setPath(m_results[index.row()].songId);
    return url;
}

QueryResultsModel *queryTekstowo(SongDescription &&songDescription)
{
    auto url = tekstowoUrl();
    url.setPath(QStringLiteral("/szukaj,wykonawca,%1,tytul,%2.html").arg(songDescription.artist, songDescription.title));
    return new TekstowoResultsModel(std::move(songDescription), Utility::networkAccessManager().get(QNetworkRequest(url)));
}

} // namespace QtGui
