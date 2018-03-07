#include "./networkaccessmanager.h"

#include <QNetworkAccessManager>

namespace Utility {

QNetworkAccessManager &networkAccessManager()
{
    static QNetworkAccessManager mgr;
    return mgr;
}

} // namespace Utility
