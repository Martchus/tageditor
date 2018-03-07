#include "./notificationmodel.h"

#include "../misc/utility.h"

#include <c++utilities/chrono/datetime.h>

#include <QApplication>
#include <QIcon>
#include <QStyle>

using namespace std;
using namespace ChronoUtilities;
using namespace TagParser;
using namespace Utility;

namespace QtGui {

DiagModel::DiagModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

QVariant DiagModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    switch (orientation) {
    case Qt::Horizontal:
        switch (role) {
        case Qt::DisplayRole:
            switch (section) {
            case 0:
                return tr("Context");
            case 1:
                return tr("Message");
            case 2:
                return tr("Time");
            default:;
            }
            break;
        }
        break;
    default:;
    }
    return QVariant();
}

int DiagModel::columnCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return 3;
    }
    return 0;
}

int DiagModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return sizeToInt(m_diag.size());
    }
    return 0;
}

Qt::ItemFlags DiagModel::flags(const QModelIndex &index) const
{
    return QAbstractListModel::flags(index);
}

QVariant DiagModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid() && index.row() >= 0 && static_cast<std::size_t>(index.row()) < m_diag.size()) {
        switch (role) {
        case Qt::DisplayRole:
            switch (index.column()) {
            case 0: {
                const string &context = m_diag[static_cast<std::size_t>(index.row())].context();
                if (context.empty()) {
                    return tr("unspecified");
                } else {
                    return QString::fromUtf8(context.c_str());
                }
            }
            case 1:
                return QString::fromUtf8(m_diag[static_cast<std::size_t>(index.row())].message().c_str());
            case 2:
                return QString::fromUtf8(
                    m_diag[static_cast<std::size_t>(index.row())].creationTime().toString(DateTimeOutputFormat::DateAndTime, true).c_str());
            default:;
            }
            break;
        case Qt::DecorationRole:
            switch (index.column()) {
            case 0:
                switch (m_diag[static_cast<std::size_t>(index.row())].level()) {
                case DiagLevel::None:
                case DiagLevel::Debug:
                    return debugIcon();
                case DiagLevel::Information:
                    return informationIcon();
                case DiagLevel::Warning:
                    return warningIcon();
                case DiagLevel::Critical:
                case DiagLevel::Fatal:
                    return errorIcon();
                }
                break;
            default:;
            }
            break;
        default:;
        }
    }
    return QVariant();
}

const Diagnostics &DiagModel::diagnostics() const
{
    return m_diag;
}

void DiagModel::setDiagnostics(const TagParser::Diagnostics &notifications)
{
    beginResetModel();
    m_diag = notifications;
    endResetModel();
}

const QIcon &DiagModel::informationIcon()
{
    static const QIcon icon = QApplication::style()->standardIcon(QStyle::SP_MessageBoxInformation);
    return icon;
}

const QIcon &DiagModel::warningIcon()
{
    static const QIcon icon = QApplication::style()->standardIcon(QStyle::SP_MessageBoxWarning);
    return icon;
}

const QIcon &DiagModel::errorIcon()
{
    static const QIcon icon = QApplication::style()->standardIcon(QStyle::SP_MessageBoxCritical);
    return icon;
}

const QIcon &DiagModel::debugIcon()
{
    static const QIcon icon = QIcon(QStringLiteral("/images/bug"));
    return icon;
}

} // namespace QtGui
