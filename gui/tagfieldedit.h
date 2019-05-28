#ifndef QTGUI_TAGFIELDLINEEDIT_H
#define QTGUI_TAGFIELDLINEEDIT_H

#include "./previousvaluehandling.h"

#include <c++utilities/application/global.h>

#include <QPair>
#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QVBoxLayout)
QT_FORWARD_DECLARE_CLASS(QLabel)

namespace TagParser {
class TagValue;
class Tag;
DECLARE_ENUM_CLASS(KnownField, unsigned int);
DECLARE_ENUM_CLASS(TagDataType, unsigned int);
DECLARE_ENUM_CLASS(TagTextEncoding, unsigned int);
} // namespace TagParser

namespace Widgets {
class ButtonOverlay;
class ClearLineEdit;
class ClearComboBox;
class ClearSpinBox;
class ClearPlainTextEdit;
class IconButton;
} // namespace Widgets

namespace QtGui {

class PicturePreviewSelection;

class TagFieldEdit : public QWidget {
    Q_OBJECT

public:
    explicit TagFieldEdit(const QList<TagParser::Tag *> &tags, TagParser::KnownField field, QWidget *parent = nullptr);

    const QList<TagParser::Tag *> &tags() const;
    TagParser::KnownField field() const;
    void setTagField(const QList<TagParser::Tag *> &tags, TagParser::KnownField field,
        PreviousValueHandling previousValueHandling = PreviousValueHandling::Clear, bool preventUiUpdate = false);
    TagParser::TagValue value(TagParser::TagTextEncoding encoding, bool includeDescription) const;
    bool setValue(const TagParser::TagValue &value, PreviousValueHandling previousValueHandling = PreviousValueHandling::Clear);
    bool hasDescription() const;
    bool canApply(TagParser::KnownField field) const;
    bool isLocked() const;
    void setLocked(bool locked);
    void toggleLocked();

public slots:
    void clear();
    void apply();
    void restore();
    void setCoverButtonsHidden(bool hideCoverButtons);

signals:
    void returnPressed();

protected:
    virtual bool eventFilter(QObject *obj, QEvent *event);

private slots:
    void handleRestoreButtonClicked();
    void handleRestoreButtonDestroyed(QObject *obj = nullptr);
    void handleLockButtonDestroyed(QObject *obj = nullptr);

private:
    TagParser::TagDataType determineDataType();
    void setupUi();
    Widgets::ClearLineEdit *setupLineEdit();
    Widgets::ClearPlainTextEdit *setupPlainTextEdit();
    Widgets::ClearComboBox *setupGenreComboBox();
    Widgets::ClearSpinBox *setupSpinBox();
    QPair<Widgets::ClearSpinBox *, Widgets::ClearSpinBox *> &setupPositionInSetSpinBoxes();
    PicturePreviewSelection *setupPictureSelection();
    QWidget *setupFileSelection();
    Widgets::ClearLineEdit *setupDescriptionLineEdit();
    QLabel *setupTypeNotSupportedLabel();
    void updateValue(PreviousValueHandling previousValueHandling = PreviousValueHandling::Clear);
    void updateValue(TagParser::Tag *tag, PreviousValueHandling previousValueHandling = PreviousValueHandling::Clear);
    void updateValue(
        const TagParser::TagValue &value, PreviousValueHandling previousValueHandling = PreviousValueHandling::Clear, bool resetRestoreButton = true);
    Widgets::IconButton *setupRestoreButton();
    Widgets::IconButton *setupLockButton();
    void showRestoreButton();
    void applyAutoCorrection(QString &textValue);
    void concretizePreviousValueHandling(PreviousValueHandling &previousValueHandling);

    QVBoxLayout *m_layout;
    const QList<TagParser::Tag *> *m_tags;
    TagParser::KnownField m_field;
    TagParser::TagDataType m_dataType;
    QList<QWidget *> m_widgets;
    Widgets::ClearLineEdit *m_lineEdit;
    Widgets::ClearComboBox *m_comboBox;
    QPair<Widgets::ClearSpinBox *, Widgets::ClearSpinBox *> m_spinBoxes;
    PicturePreviewSelection *m_pictureSelection;
    Widgets::ClearPlainTextEdit *m_plainTextEdit;
    Widgets::ClearLineEdit *m_descriptionLineEdit;
    Widgets::IconButton *m_restoreButton;
    Widgets::IconButton *m_lockButton;
    bool m_isLocked;
};

inline const QList<TagParser::Tag *> &TagFieldEdit::tags() const
{
    return *m_tags;
}

inline TagParser::KnownField TagFieldEdit::field() const
{
    return m_field;
}

inline bool TagFieldEdit::isLocked() const
{
    return m_isLocked;
}

inline void TagFieldEdit::toggleLocked()
{
    setLocked(!isLocked());
}

} // namespace QtGui

#endif // QTGUI_TAGFIELDLINEEDIT_H
