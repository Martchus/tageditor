#include "./tagedit.h"
#include "./tagfieldedit.h"

#include "../application/knownfieldmodel.h"
#include "../application/settings.h"

#include <tagparser/tag.h>

#include <qtutilities/misc/conversion.h>

#include <c++utilities/conversion/stringconversion.h>

#include <QFormLayout>
#include <QLabel>
#include <QSplitter>
#include <QVBoxLayout>

using namespace std;
using namespace std::placeholders;
using namespace CppUtilities;
using namespace TagParser;

namespace QtGui {

/*!
 * \class QtGui::TagEdit
 * \brief The TagEdit widget allows the user to edit TagParser::Tag objects.
 */

/*!
 * \brief Constructs a new TagEdit object.
 */
TagEdit::TagEdit(QWidget *parent)
    : QWidget(parent)
{
    auto *const mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    QSplitter *const splitter = new QSplitter(this);
    splitter->setOrientation(Qt::Horizontal);
    auto *widget = new QWidget(this);
    m_layoutLeft = new QFormLayout(widget);
    m_layoutLeft->setContentsMargins(QMargins());
    m_layoutLeft->setSpacing(2);
    widget->setLayout(m_layoutLeft);
    splitter->addWidget(widget);
    widget = new QWidget(this);
    m_layoutRight = new QVBoxLayout(widget);
    m_layoutRight->setContentsMargins(QMargins());
    widget->setLayout(m_layoutRight);
    splitter->addWidget(widget);
    splitter->setSizes(QList<int>() << 500);
    mainLayout->addWidget(splitter);
    setLayout(mainLayout);
}

/*!
 * \brief Returns the current value for the specified \a field.
 * \remarks Doesn't work for fields of the type picture.
 */
TagValue TagEdit::value(KnownField field, TagTextEncoding encoding) const
{
    if (const TagFieldEdit *const edit = m_widgets.value(field, nullptr)) {
        return edit->value(encoding, false);
    } else {
        return TagValue();
    }
}

std::int32_t TagEdit::trackNumber() const
{
    std::int32_t trackNumber = 0;
    try {
        trackNumber = value(KnownField::TrackPosition).toPositionInSet().position();
    } catch (const ConversionException &) {
    }
    if (trackNumber) {
        return trackNumber;
    }
    for (const auto *const tag : tags()) {
        if (tag->supportsTarget() && tag->targetLevel() != TagTargetLevel::Track) {
            continue;
        }
        try {
            trackNumber = value(KnownField::PartNumber).toInteger();
        } catch (const ConversionException &) {
        }
        if (trackNumber) {
            return trackNumber;
        }
    }
    return trackNumber;
}

int32_t TagEdit::diskNumber() const
{
    std::int32_t diskNumber = 0;
    try {
        diskNumber = value(KnownField::DiskPosition).toPositionInSet().position();
    } catch (const ConversionException &) {
    }
    if (diskNumber) {
        return diskNumber;
    }
    for (const auto *const tag : tags()) {
        if (!tag->supportsTarget() || tag->targetLevel() != TagTargetLevel::Part) {
            continue;
        }
        try {
            diskNumber = value(KnownField::PartNumber).toInteger();
        } catch (const ConversionException &) {
        }
        if (diskNumber) {
            return diskNumber;
        }
    }
    return diskNumber;
}

/*!
 * \brief Assigns the specified \a tag to the edit.
 * \param updateUi Specifies whether the UI of should be updated.
 * \remarks The TagEdit object does not take ownership.
 */
void TagEdit::setTag(Tag *tag, bool updateUi)
{
    m_tags.clear();
    if (tag) {
        m_tags << tag;
    }
    if (updateUi) {
        setupUi();
    } else {
        assignTags();
    }
}

/*!
 * \brief Assigns the specified \a tags to the edit.
 * \param updateUi Specifies whether the UI of should be updated.
 * \remarks The TagEdit object does not take ownership.
 */
void TagEdit::setTags(const QList<Tag *> &tags, bool updateUi)
{
    m_tags = tags;
    if (updateUi) {
        setupUi();
    } else {
        assignTags();
    }
}

/*!
 * \brief Sets the \a value for the specified \a field manually applying the specified \a previousValueHandling.
 *
 * Used for editing tags programmatically, eg. in TagEditorWidget::insertTitleFromFilename() and DbQueryWidget::applyResults().
 */
bool TagEdit::setValue(KnownField field, const TagParser::TagValue &value, PreviousValueHandling previousValueHandling)
{
    if (TagFieldEdit *const edit = m_widgets.value(field, nullptr)) {
        return edit->setValue(value, previousValueHandling);
    } else {
        return false;
    }
}

/*!
 * \brief Returns whether at least on of the assigned tags supports the specified \a field.
 */
bool TagEdit::hasField(KnownField field) const
{
    for (Tag *const tag : m_tags) {
        if (tag->supportsField(field)) {
            return true;
        }
    }
    return false;
}

bool TagEdit::hasAutoCorrectionBeenApplied() const
{
    for (auto i = m_widgets.constBegin(), end = m_widgets.constEnd(); i != end; ++i) {
        if (i.value()->hasAutoCorrectionBeenApplied()) {
            return true;
        }
    }
    return false;
}

/*!
 * \brief Generates a label for the currently assigned tags.
 *
 * (Used as combo box item for the tag edit.)
 */
QString TagEdit::generateLabel() const
{
    if (m_tags.isEmpty()) {
        return QString();
    }
    const TagTarget &target = m_tags.at(0)->target();
    bool differentTargets = false, haveMatroskaTags = false;
    QStringList tagNames;
    tagNames.reserve(m_tags.size());
    for (const Tag *const tag : m_tags) {
        tagNames << QtUtilities::qstringFromStdStringView(tag->typeName());
        if (!differentTargets && !(target == tag->target())) {
            differentTargets = true;
        }
        if (tag->type() == TagType::MatroskaTag) {
            haveMatroskaTags = true;
        }
    }
    QString res = tagNames.join(QStringLiteral(", "));
    if (differentTargets) {
        res.append(tr(" with different targets"));
    } else if (haveMatroskaTags || !target.isEmpty()) {
        res.append(tr(" targeting %1").arg(QString::fromStdString(m_tags.front()->targetString())));
    }
    return res;
}

/*!
 * \brief Clears all TagFieldEdit widgets.
 */
void TagEdit::clear()
{
    for (TagFieldEdit *const edit : m_widgets) {
        edit->clear();
    }
}

/*!
 * \brief Restores all TagFieldEdit widgets.
 */
void TagEdit::restore()
{
    for (TagFieldEdit *const edit : m_widgets) {
        edit->restore();
    }
}

/*!
 * \brief Applies all entered values to the assigned tags.
 */
void TagEdit::apply()
{
    switch (Settings::values().tagPocessing.unsupportedFieldHandling) {
    case Settings::UnsupportedFieldHandling::Discard:
        // remove all old fields of all tags to discard
        // all unsupported values
        for (Tag *const tag : m_tags) {
            tag->removeAllFields();
        }
        break;
    default:;
    }
    for (TagFieldEdit *const edit : m_widgets) {
        edit->apply();
    }
}

/*!
 * \brief Invalidates the UI.
 *
 * All widgets are removed and recreated. Hence all fields are restored
 * to the initial values. Assigned tags will remain.
 */
void TagEdit::invalidate()
{
    // remove current widgets
    for (QWidget *const edit : m_widgets) {
        removeEdit(edit);
    }
    m_widgets.clear();
    // recreate widgets
    setupUi();
}

/*!
 * \brief Sets whether cover buttons are hidden.
 */
void TagEdit::setCoverButtonsHidden(bool hideCoverButtons)
{
    for (auto i = m_widgets.begin(), end = m_widgets.end(); i != end; ++i) {
        i.value()->setCoverButtonsHidden(hideCoverButtons);
    }
}

/*!
 * \brief Internally called to setup the UI.
 */
void TagEdit::setupUi()
{
    setUpdatesEnabled(false);
    if (m_tags.empty()) {
        // there are no tags assigned -> remove all editing controls
        for (QWidget *const edit : m_widgets) {
            removeEdit(edit);
        }
        m_widgets.clear();
        setUpdatesEnabled(true);
        return;
    }

    // there are tags assigned
    // setup editing controls
    TagFieldEdit *edit = nullptr;
    int rowOverall = 0, rowLeft = 0, rowRight = 0;
    for (const auto &item : Settings::values().editor.fields.items()) {
        const auto field = static_cast<KnownField>(item.id().toInt());
        if (!item.isChecked() || !hasField(field)) {
            // the field is either disabled or it is not supported by at least one of the assigned tags
            if ((edit = m_widgets.value(field))) {
                m_widgets.remove(field);
                removeEdit(edit);
            }
            continue;
        }

        // the field is not disabled and the field is supported by at least one of the assigned tags
        edit = m_widgets.value(field, nullptr);
        if (edit) {
            // we have already an edit for the field -> try to recycle it
            // the order might have changed
            int prevIndex; // stores the previous index (NOT row)
            switch (field) {
            case KnownField::Cover:
            case KnownField::Lyrics:
                // these fields are shown at the right side
                prevIndex = m_layoutRight->indexOf(edit);
                if (prevIndex > 0 && (rowRight * 2 + 1) != prevIndex) {
                    QLayoutItem *item1 = m_layoutRight->itemAt(prevIndex - 1);
                    QLayoutItem *item2 = m_layoutRight->itemAt(prevIndex);
                    m_layoutRight->removeItem(item1);
                    m_layoutRight->removeItem(item2);
                    m_layoutRight->insertItem(rowRight * 2, item1);
                    m_layoutRight->insertItem(rowRight * 2 + 1, item2);
                }
                break;
            default:
                // the other fields are shown at the left side
                prevIndex = m_layoutLeft->indexOf(edit);
                if (prevIndex >= 0 && (rowLeft * 2 + 1) != prevIndex) {
                    QLayoutItem *item1 = m_layoutLeft->itemAt(prevIndex - 1);
                    QLayoutItem *item2 = m_layoutLeft->itemAt(prevIndex);
                    QWidget *label = item1->widget();
                    m_layoutLeft->removeItem(item1);
                    m_layoutLeft->removeItem(item2);
                    delete item1;
                    delete item2;
                    m_layoutLeft->insertRow(rowLeft, label, edit);
                }
            }
            // update the tag field
            edit->setTagField(m_tags, field, m_previousValueHandling);
        } else {
            // we need to create a new edit for the field
            edit = new TagFieldEdit(m_tags, field, this);
            connect(edit, &TagFieldEdit::returnPressed, this, &TagEdit::returnPressed);
            switch (field) {
            case KnownField::Cover:
            case KnownField::Lyrics:
                // editing widgets for these fields will be show at the right side (m_layoutRight)
                m_layoutRight->insertWidget(rowRight * 2, new QLabel(item.label(), this));
                edit->setContentsMargins(10, 0, 0, 0);
                m_layoutRight->insertWidget(rowRight * 2 + 1, edit);
                break;
            default:
                // editing widgets for the other fields will be show at the left side (m_layoutLeft)
                m_layoutLeft->insertRow(rowLeft, item.label(), edit);
            }
            m_widgets.insert(field, edit);
        }
        // update the current position
        // note: The position is required to keep the order of Settings::fieldModel().fields().
        ++rowOverall;
        switch (field) {
        case KnownField::Cover:
        case KnownField::Lyrics:
            ++rowRight;
            break;
        default:
            ++rowLeft;
        }
    }
    setUpdatesEnabled(true);
}

/*!
 * \brief Internally called to delete an edit removing it (and its label) from its current layout.
 */
void TagEdit::removeEdit(QWidget *edit)
{
    edit->deleteLater();
    // the left layout might contain the edit
    const auto leftIndex = m_layoutLeft->indexOf(edit);
    if (leftIndex > 0) {
        // delete label as well
        if (QWidget *const label = m_layoutLeft->labelForField(edit)) {
            label->deleteLater();
        }
        m_layoutLeft->removeWidget(edit);
        return;
    }
    // or the right layout might contain the edit
    const auto rightIndex = m_layoutRight->indexOf(edit);
    if (rightIndex < 0) {
        return;
    }
    // delete label as well
    QLayoutItem *const labelItem = m_layoutRight->itemAt(rightIndex - 1);
    m_layoutRight->removeWidget(edit);
    if (!labelItem || !labelItem->widget()) {
        return;
    }
    labelItem->widget()->deleteLater();
    m_layoutRight->removeWidget(labelItem->widget());
}

/*!
 * \brief Internlly called to assign tags without updating the UI.
 */
void TagEdit::assignTags()
{
    for (TagFieldEdit *const edit : m_widgets) {
        edit->setTagField(m_tags, edit->field(), m_previousValueHandling, true);
    }
}

} // namespace QtGui
