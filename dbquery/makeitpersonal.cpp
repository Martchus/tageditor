#include "./makeitpersonal.h"

#include "../application/settings.h"
#include "../misc/networkaccessmanager.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QStringBuilder>
#include <QUrlQuery>

#include <functional>

using namespace std;
using namespace std::placeholders;
using namespace Utility;

namespace QtGui {

static const QString defaultMakeItPersonalUrl(QStringLiteral("https://makeitpersonal.co"));

static QUrl makeItPersonalApiUrl()
{
    const auto &makeItPersonalUrl = Settings::values().dbQuery.makeItPersonalUrl;
    return QUrl((makeItPersonalUrl.isEmpty() ? defaultMakeItPersonalUrl : makeItPersonalUrl) + QStringLiteral("/lyrics"));
}

MakeItPersonalResultsModel::MakeItPersonalResultsModel(SongDescription &&initialSongDescription, QNetworkReply *reply)
    : HttpResultsModel(std::move(initialSongDescription), reply)
{
}

bool MakeItPersonalResultsModel::fetchLyrics(const QModelIndex &index)
{
    Q_UNUSED(index)
    return true; // lyrics don't need to be fetched separately here
}

void MakeItPersonalResultsModel::parseInitialResults(const QByteArray &data)
{
    // prepare parsing meta data
    beginResetModel();
    m_results.clear();

    SongDescription desc = m_initialDescription;
    desc.songId = m_initialDescription.artist + m_initialDescription.title;
    desc.artistId = m_initialDescription.artist;
    desc.lyrics = QString::fromUtf8(data).trimmed();
    if (desc.lyrics != QLatin1String("Sorry, We don't have lyrics for this song yet.")) {
        m_results << std::move(desc);
    }

    // promote changes
    endResetModel();
}

QueryResultsModel *queryMakeItPersonal(SongDescription &&songDescription)
{
    // compose URL
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("artist"), songDescription.artist);
    query.addQueryItem(QStringLiteral("title"), songDescription.title);
    QUrl url(makeItPersonalApiUrl());
    url.setQuery(query);

    // make request
    return new MakeItPersonalResultsModel(std::move(songDescription), Utility::networkAccessManager().get(QNetworkRequest(url)));
}

} // namespace QtGui
