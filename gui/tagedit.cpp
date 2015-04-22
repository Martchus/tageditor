#include "tagedit.h"
#include "tagfieldedit.h"

#include "application/settings.h"
#include "application/knownfieldmodel.h"

#include <tagparser/tag.h>

#include <c++utilities/conversion/stringconversion.h>

#include <QSplitter>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QLabel>

using namespace std;
using namespace std::placeholders;
using namespace ConversionUtilities;
using namespace Media;

namespace QtGui {

/*
    TRANSLATOR QtGui::TagEdit
    Necessary for lupdate.
*/

/*!
 * \class QtGui::TagEdit
 * \brief The TagEdit widget allows the user to edit Media::Tag objects.
 */

/*!
 * \brief Constructs a new TagEdit object.
 */
TagEdit::TagEdit(QWidget *parent) :
    QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setMargin(0);
    QSplitter *splitter = new QSplitter(this);
    splitter->setOrientation(Qt::Horizontal);
    QWidget *widget = new QWidget(this);
    m_layoutLeft = new QFormLayout(widget);
    m_layoutLeft->setContentsMargins(QMargins());
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
 * \brief Destroys the tag edit.
 *
 * Does not destroy assigned tags.
 */
TagEdit::~TagEdit()
{}

/*!
 * \brief Assigns the specified \a tag to the edit.
 * \param updateUi Specifies whether the UI of should be updated.
 * \remarks The TagEdit object does not take ownership.
 */
void TagEdit::setTag(Tag *tag, bool updateUi)
{
    m_tags.clear();
    if(tag) {
        m_tags << tag;
    }
    if(updateUi) {
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
    if(updateUi) {
        setupUi();
    } else {
        assignTags();
    }
}

/*!
 * \brief Sets the \a value for the specified \a field manually applying the specified \a previousValueHandling.
 */
bool TagEdit::setValue(KnownField field, const Media::TagValue &value, PreviousValueHandling previousValueHandling)
{
    TagFieldEdit *edit = m_widgets.value(field, nullptr);
    if(edit) {
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
    foreach(Tag *tag, m_tags) {
        if(tag->supportsField(field)) {
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
    if(!m_tags.isEmpty()) {
        TagTarget target = m_tags.at(0)->target();
        bool differentTargets = false;
        QStringList tagNames;
        foreach(Tag *tag, m_tags) {
            tagNames << QString::fromLocal8Bit(tag->typeName());
            if(!differentTargets && !(target == tag->target())) {
                differentTargets = true;
            }
        }
        QString res = tagNames.join(QStringLiteral(", "));
        if(differentTargets) {
            res.append(tr(" with different targets"));
        } else {
            if(!target.isEmpty()) {
                res.append(tr(" targeting %1").arg(QString::fromLocal8Bit(target.toString().c_str())));
            }
        }
        return res;
    }
    return QString();
}

/*!
 * \brief Clears all TagFieldEdit widgets.
 */
void TagEdit::clear()
{
    foreach(TagFieldEdit *edit, m_widgets) {
        edit->clear();
    }
}

/*!
 * \brief Restores all TagFieldEdit widgets.
 */
void TagEdit::restore()
{
    foreach(TagFieldEdit *edit, m_widgets) {
        edit->restore();
    }
}

/*!
 * \brief Applies all entered values to the assigned tags.
 */
void TagEdit::apply()
{
    switch(Settings::unsupportedFieldHandling()) {
    case Settings::UnsupportedFieldHandling::Discard:
        // remove all old fields of all tags to discard
        // all unsupported values
        foreach(Tag *tag, m_tags) {
            tag->removeAllFields();
        }
        break;
    default:
        ;
    }
    foreach(TagFieldEdit *edit, m_widgets) {
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
    foreach(QWidget *edit, m_widgets) {
        removeEdit(edit);
        edit->deleteLater();
    }
    m_widgets.clear();
    // recreate widgets
    setupUi();
}

/*!
 * \brief Internally called to setup the UI.
 */
void TagEdit::setupUi()
{
    setUpdatesEnabled(false);
    if(!m_tags.size()) {
        // there are no tags assigned -> remove all editing controls
        foreach(QWidget *edit, m_widgets) {
            removeEdit(edit);
            edit->deleteLater();
        }
        m_widgets.clear();
    } else {
        // there are tags assigned
//        if(m_tags.size() > 1) {
//            // display targets
//            list<string> targets;
//            for(const Tag *tag : m_tags) {
//                if(tag->supportsTarget()) {
//                    const TagTarget &target = tag->target();
//                    if(!target.levelName().empty()) {
//                        targets.push_back(target.levelName());
//                    } else {
//                        targets.push_back(numberToString(target.level()));
//                    }
//                }
//            }
//            if(targets.size()) {
//                QString res = QString::fromStdString(joinStrings(targets, ", ", true));

//            }
//        }
        // setup editing controls
        TagFieldEdit *edit = nullptr;
        int rowOverall = 0, rowLeft = 0, rowRight = 0;
        for(const auto &item : Settings::selectedFieldsModel().items()) {
            KnownField field = static_cast<KnownField>(item.id().toInt());
            if(item.isChecked() && hasField(field)) {
                // the field is not disabled and the field is supported by at least one of the assigned tags
                edit = m_widgets.value(field, nullptr);
                if(edit) {
                    // we have already an edit for the field -> try to recycle it
                    // the order might have changed
                    int prevIndex; // stores the previous index (NOT row)
                    switch(field) {
                    case KnownField::Cover: case KnownField::Lyrics:
                        // these fields are shown at the right side
                        prevIndex = m_layoutRight->indexOf(edit);
                        if(prevIndex > 0 && (rowRight * 2 + 1) != prevIndex) {
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
                        if(prevIndex > 0 && (rowLeft * 2 + 1) != prevIndex) {
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
                    switch(field) {
                    case KnownField::Cover: case KnownField::Lyrics:
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
                // the position is required to keep the order of Settings::fieldModel().fields()
                ++rowOverall;
                switch(field) {
                case KnownField::Cover:
                case KnownField::Lyrics:
                    ++rowRight;
                    break;
                default:
                    ++rowLeft;
                }
            } else {
                // the field is either disabled or it is not supported by at least one of the assigned tags
                if((edit = m_widgets.value(field))) {
                    m_widgets.remove(field);
                    removeEdit(edit);
                    edit->deleteLater();
                }
            }
        }
    }
    setUpdatesEnabled(true);
}

/*!
 * \brief Internally called to remove an edit.
 */
void TagEdit::removeEdit(QWidget *edit)
{
    // delete label of the widget (if present) as well
    // the left layout might contain the label
    if(QWidget *label = m_layoutLeft->labelForField(edit)) {
        label->deleteLater();
    }
    // or the right layout might contain the label
    int i = m_layoutRight->indexOf(edit) - 1;
    if(i >= 0) {
        if(QLayoutItem *item = m_layoutRight->itemAt(i)) {
            if(item->widget()) {
                item->widget()->deleteLater();
                m_layoutRight->removeWidget(item->widget());
            }
        }
    }
}

/*!
 * \brief Internlly called to assign tags without updating the UI.
 */
void TagEdit::assignTags()
{
    foreach(TagFieldEdit *edit, m_widgets) {
        edit->setTagField(m_tags, edit->field(), m_previousValueHandling, true);
    }
}

}
