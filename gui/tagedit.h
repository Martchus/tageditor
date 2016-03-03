#ifndef QTGUI_TAGEDIT_H
#define QTGUI_TAGEDIT_H

#include "./previousvaluehandling.h"

#include <QList>
#include <QMap>
#include <QWidget>

#include <initializer_list>

QT_BEGIN_NAMESPACE
class QFormLayout;
class QVBoxLayout;
QT_END_NAMESPACE

namespace Media {
class Tag;
class TagValue;
enum class KnownField : unsigned int;
}

namespace QtGui {

class TagFieldEdit;

class TagEdit : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(PreviousValueHandling previousValueHandling READ previousValueHandling WRITE setPreviousValueHandling)

public:
    explicit TagEdit(QWidget *parent = nullptr);
    const QList<Media::Tag *> &tags() const;
    Media::TagValue value(Media::KnownField field) const;
    void setTag(Media::Tag *tag, bool updateUi = true);
    void setTags(const QList<Media::Tag *> &tags, bool updateUi = true);
    bool setValue(Media::KnownField field, const Media::TagValue &value, PreviousValueHandling previousValueHandling = PreviousValueHandling::Clear);
    template<class Container = std::initializer_list<Media::Tag *> > void setTags(const Container &tags, bool updateUi = true);
    bool hasField(Media::KnownField field) const;
    PreviousValueHandling previousValueHandling() const;
    void setPreviousValueHandling(PreviousValueHandling previousValueHandling);
    TagFieldEdit *tagFieldEdit(Media::KnownField field) const;
    QString generateLabel() const;

public slots:
    void clear();
    void restore();
    void apply();
    void invalidate();

signals:
    void returnPressed();
    
private:
    void setupUi();
    void removeEdit(QWidget *edit);
    void assignTags();

    QList<Media::Tag *> m_tags;
    QFormLayout *m_layoutLeft;
    QVBoxLayout *m_layoutRight;
    QMap<Media::KnownField, TagFieldEdit *> m_widgets;
    PreviousValueHandling m_previousValueHandling;
};

/*!
 * \brief Returns the currently assigned tags.
 */
inline const QList<Media::Tag *> &TagEdit::tags() const
{
    return m_tags;
}

/*!
 * \brief Assigns the specified \a tags to the edit.
 * \param updateUi Specifies whether the UI of should be updated.
 * \remarks The TagEdit object does not take ownership.
 */
template<class Container> void TagEdit::setTags(const Container &tags, bool updateUi)
{
    m_tags.clear();
    m_tags.reserve(tags.size());
    for(auto *tag : tags) {
        m_tags << tag;
    }
    if(updateUi) {
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
inline TagFieldEdit *TagEdit::tagFieldEdit(Media::KnownField field) const
{
    return m_widgets.value(field, nullptr);
}

}

#endif // QTGUI_TAGEDIT_H
