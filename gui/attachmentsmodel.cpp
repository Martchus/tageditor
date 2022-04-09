#include "./attachmentsmodel.h"

#include <tagparser/abstractattachment.h>

#include <c++utilities/conversion/stringconversion.h>

#include <QApplication>

using namespace CppUtilities;
using namespace TagParser;

namespace QtGui {

AttachmentItem::AttachmentItem(AbstractAttachment *attachment, bool activated, const QString &location)
    : m_attachment(attachment)
    , m_location(location)
{
    revert();
    m_activated = activated;
    if (m_attachment->data()) {
        m_size = QString::fromStdString(dataSizeToString(attachment->data()->size()));
    } else {
        m_size = QApplication::translate("AttachmentItem", "n/a");
    }
    if (m_location.isEmpty()) {
        m_location = QApplication::translate("AttachmentItem", "currently attached");
    }
}

TagParser::AbstractAttachment *AttachmentItem::attachment()
{
    return m_attachment;
}

const TagParser::AbstractAttachment *AttachmentItem::attachment() const
{
    return m_attachment;
}

const QString &AttachmentItem::name() const
{
    return m_name;
}

void AttachmentItem::setName(const QString &name)
{
    m_name = name;
}

const QString &AttachmentItem::description() const
{
    return m_description;
}

void AttachmentItem::setDescription(const QString &description)
{
    m_description = description;
}

const QString &AttachmentItem::mimeType() const
{
    return m_mimeType;
}

void AttachmentItem::setMimeType(const QString &mimeType)
{
    m_mimeType = mimeType;
}

const QString &AttachmentItem::size() const
{
    return m_size;
}

const QString &AttachmentItem::location() const
{
    return m_location;
}

bool AttachmentItem::isActivated() const
{
    return m_activated;
}

void AttachmentItem::setActivated(bool activated)
{
    m_activated = activated;
}

void AttachmentItem::revert()
{
    m_name = QString::fromStdString(m_attachment->name());
    m_description = QString::fromStdString(m_attachment->description());
    m_mimeType = QString::fromStdString(m_attachment->mimeType());
    m_activated = !m_attachment->isIgnored();
}

void AttachmentItem::submit()
{
    m_attachment->setName(m_name.toUtf8().toStdString());
    m_attachment->setDescription(m_description.toUtf8().toStdString());
    m_attachment->setMimeType(m_mimeType.toUtf8().toStdString());
    m_attachment->setIgnored(!m_activated);
}

AttachmentsModel::AttachmentsModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

AttachmentsModel::~AttachmentsModel()
{
}

QVariant AttachmentsModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid() && index.row() < m_attachments.size()) {
        const auto &item = m_attachments.at(index.row());
        switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
            switch (index.column()) {
            case 0:
                return item.name();
            case 1:
                return item.description();
            case 2:
                return item.mimeType();
            case 3:
                return item.size();
            case 4:
                return item.location();
            default:;
            }
            break;
        case Qt::CheckStateRole:
            switch (index.column()) {
            case 0:
                return QVariant(static_cast<int>(item.isActivated() ? Qt::Checked : Qt::Unchecked));
            default:;
            }
        default:;
        }
    }
    return QVariant();
}

bool AttachmentsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.isValid() && index.row() < m_attachments.size()) {
        auto &item = m_attachments[index.row()];
        switch (role) {
        case Qt::EditRole:
            switch (index.column()) {
            case 0:
                item.setName(value.toString());
                emit dataChanged(index, index, QVector<int>() << role);
                return true;
            case 1:
                item.setDescription(value.toString());
                emit dataChanged(index, index, QVector<int>() << role);
                return true;
            case 2:
                item.setMimeType(value.toString());
                emit dataChanged(index, index, QVector<int>() << role);
                return true;
            default:;
            }
            break;
        case Qt::CheckStateRole:
            switch (index.column()) {
            case 0:
                item.setActivated(value.toInt() == Qt::Checked);
                emit dataChanged(index, index, QVector<int>() << role);
                return true;
            default:;
            }
            break;
        default:;
        }
    }
    return false;
}

Qt::ItemFlags AttachmentsModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = Qt::ItemNeverHasChildren | Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    if (index.isValid()) {
        switch (index.column()) {
        case 0:
            flags |= Qt::ItemIsUserCheckable;
            [[fallthrough]];
        case 1:
        case 2:
            flags |= Qt::ItemIsEditable;
            break;
        default:;
        }
    }
    return flags;
}

QVariant AttachmentsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    switch (orientation) {
    case Qt::Horizontal:
        switch (role) {
        case Qt::DisplayRole:
            switch (section) {
            case 0:
                return tr("Name");
            case 1:
                return tr("Description");
            case 2:
                return tr("MIME-type");
            case 3:
                return tr("Size");
            case 4:
                return tr("Location");
            default:;
            }
            break;
        default:;
        }
        break;
    default:;
    }
    return QVariant();
}

int AttachmentsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    } else {
        return m_attachments.size();
    }
}

int AttachmentsModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    } else {
        return 5;
    }
}

void AttachmentsModel::revert()
{
    for (auto &item : m_attachments) {
        item.revert();
    }
    emit dataChanged(index(0, 0), index(m_attachments.size() - 1, 0), QVector<int>() << Qt::CheckStateRole);
}

bool AttachmentsModel::submit()
{
    for (auto &item : m_attachments) {
        item.submit();
    }
    return true;
}

void AttachmentsModel::repealSelection()
{
    if (!m_attachments.isEmpty()) {
        for (auto &item : m_attachments) {
            item.setActivated(false);
        }
        emit dataChanged(index(0, 0), index(m_attachments.size() - 1, 0), QVector<int>() << Qt::CheckStateRole);
    }
}

AbstractAttachment *AttachmentsModel::attachment(const QModelIndex &index)
{
    if (index.isValid() && index.row() < m_attachments.size()) {
        return m_attachments[index.row()].attachment();
    }
    return nullptr;
}

void AttachmentsModel::addAttachment(int row, AbstractAttachment *attachment, bool activated, const QString &location)
{
    if (row < 0 || row > m_attachments.size()) {
        row = m_attachments.size();
    }
    beginInsertRows(QModelIndex(), row, row);
    m_attachments.insert(row, AttachmentItem(attachment, activated, location));
    endInsertRows();
}

void AttachmentsModel::setAttachments(const QList<AbstractAttachment *> &attachments, bool activated, const QString &location)
{
    beginResetModel();
    m_attachments.clear();
    m_attachments.reserve(attachments.size());
    for (auto *attachment : attachments) {
        m_attachments << AttachmentItem(attachment, activated, location);
    }
    endResetModel();
}

void AttachmentsModel::removeAttachments(int firstRow, int lastRow)
{
    if (firstRow <= lastRow && firstRow >= 0 && firstRow < m_attachments.size() && lastRow >= 0 && lastRow < m_attachments.size()) {
        beginRemoveRows(QModelIndex(), firstRow, lastRow);
        m_attachments.erase(m_attachments.begin() + firstRow, m_attachments.end() + lastRow);
        endRemoveRows();
    }
}

} // namespace QtGui
