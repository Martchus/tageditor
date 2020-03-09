#ifndef QTGUI_TAGEDIT_H
#define QTGUI_TAGEDIT_H

#include "./previousvaluehandling.h"

#include <tagparser/tagvalue.h>

#include <QList>
#include <QMap>
#include <QWidget>

#include <initializer_list>

QT_FORWARD_DECLARE_CLASS(QFormLayout)
QT_FORWARD_DECLARE_CLASS(QVBoxLayout)

#define TAGEDITOR_ENUM_CLASS enum class
namespace TagParser {
class Tag;
class TagValue;
TAGEDITOR_ENUM_CLASS KnownField : unsigned int;
} // namespace TagParser
#undef TAGEDITOR_ENUM_CLASS

namespace QtGui {

class TagFieldEdit;

class TagEdit : public QWidget {
    Q_OBJECT
    Q_PROPERTY(PreviousValueHandling previousValueHandling READ previousValueHandling WRITE setPreviousValueHandling)

public:
    explicit TagEdit(QWidget *parent = nullptr);
    const QList<TagParser::Tag *> &tags() const;
    TagParser::TagValue value(TagParser::KnownField field, TagParser::TagTextEncoding encoding = TagParser::TagTextEncoding::Utf16LittleEndian) const;
    std::int32_t trackNumber() const;
    std::int32_t diskNumber() const;
    void setTag(TagParser::Tag *tag, bool updateUi = true);
    void setTags(const QList<TagParser::Tag *> &tags, bool updateUi = true);
    bool setValue(
        TagParser::KnownField field, const TagParser::TagValue &value, PreviousValueHandling previousValueHandling = PreviousValueHandling::Clear);
    template <class Container = std::initializer_list<TagParser::Tag *>> void setTags(const Container &tags, bool updateUi = true);
    bool hasField(TagParser::KnownField field) const;
    PreviousValueHandling previousValueHandling() const;
    void setPreviousValueHandling(PreviousValueHandling previousValueHandling);
    TagFieldEdit *tagFieldEdit(TagParser::KnownField field) const;
    QString generateLabel() const;
    bool hasAutoCorrectionBeenApplied() const;

public Q_SLOTS:
    void clear();
    void restore();
    void apply();
    void invalidate();
    void setCoverButtonsHidden(bool hideCoverButtons);

Q_SIGNALS:
    void returnPressed();

private:
    void setupUi();
    void removeEdit(QWidget *edit);
    void assignTags();

    QList<TagParser::Tag *> m_tags;
    QFormLayout *m_layoutLeft;
    QVBoxLayout *m_layoutRight;
    QMap<TagParser::KnownField, TagFieldEdit *> m_widgets;
    PreviousValueHandling m_previousValueHandling;
};

/*!
 * \brief Returns the currently assigned tags.
 */
inline const QList<TagParser::Tag *> &TagEdit::tags() const
{
    return m_tags;
}

/*!
 * \brief Assigns the specified \a tags to the edit.
 * \param updateUi Specifies whether the UI of should be updated.
 * \remarks The TagEdit object does not take ownership.
 */
template <class Container> void TagEdit::setTags(const Container &tags, bool updateUi)
{
    m_tags.clear();
    m_tags.reserve(tags.size());
    for (auto *tag : tags) {
        m_tags << tag;
    }
    if (updateUi) {
        setupUi();
    } else {
        assignTags();
    }
}

/*!
 * \brief Returns the "previous value handling".
 *
 * The "previous value handling" defines what happens which the current input
 * when the setTag() or the setTags() method is called.
 *
 * The setValue() method will not care about this setting but allows to specify
 * a "previous value handling", too.
 *
 * \sa setPreviousValueHandling()
 */
inline PreviousValueHandling TagEdit::previousValueHandling() const
{
    return m_previousValueHandling;
}

/*!
 * \brief Sets the "previous value handling".
 * \sa previousValueHandling()
 */
inline void TagEdit::setPreviousValueHandling(PreviousValueHandling previousValueHandling)
{
    m_previousValueHandling = previousValueHandling;
}

/*!
 * \brief Returns the TagFieldEdit object for the specified \a field.
 * \returns Returns a pointer to the object (ownership remains by the TagEdit object) or - if
 *          there is no TagFieldEdit object - nullptr.
 */
inline TagFieldEdit *TagEdit::tagFieldEdit(TagParser::KnownField field) const
{
    return m_widgets.value(field, nullptr);
}

} // namespace QtGui

#endif // QTGUI_TAGEDIT_H
