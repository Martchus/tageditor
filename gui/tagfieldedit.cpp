#include "./tagfieldedit.h"
#include "./picturepreviewselection.h"

#include "../application/knownfieldmodel.h"
#include "../application/settings.h"

#include "../misc/utility.h"

#include <tagparser/id3/id3v2tag.h>
#include <tagparser/mediafileinfo.h>
#include <tagparser/tag.h>
#include <tagparser/tagvalue.h>

#include <qtutilities/misc/conversion.h>
#include <qtutilities/widgets/clearcombobox.h>
#include <qtutilities/widgets/clearlineedit.h>
#include <qtutilities/widgets/clearplaintextedit.h>
#include <qtutilities/widgets/clearspinbox.h>
#include <qtutilities/widgets/iconbutton.h>

#include <c++utilities/conversion/conversionexception.h>

#include <QAction>
#include <QCursor>
#include <QEvent>
#include <QFile>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QGraphicsView>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QImage>
#include <QKeyEvent>
#include <QLabel>
#include <QListIterator>
#include <QMenu>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include <algorithm>
#include <functional>
#include <initializer_list>
#include <iostream>

using namespace std;
using namespace CppUtilities;
using namespace QtUtilities;
using namespace TagParser;

namespace QtGui {

/*!
 * \class QtGui::TagFieldEdit
 * \brief The TagFieldEdit widget allows the user to edit a specified tag field.
 */

/*!
 * \brief Constructs a new TagFieldEdit.
 * \sa setTagField()
 */
TagFieldEdit::TagFieldEdit(const QList<TagParser::Tag *> &tags, TagParser::KnownField field, QWidget *parent)
    : QWidget(parent)
    , m_layout(new QVBoxLayout(this))
    , m_tags(&tags)
    , m_field(field)
    , m_dataType(determineDataType())
    , m_lineEdit(nullptr)
    , m_comboBox(nullptr)
    , m_spinBoxes(QPair<QtUtilities::ClearSpinBox *, QtUtilities::ClearSpinBox *>(nullptr, nullptr))
    , m_pictureSelection(nullptr)
    , m_plainTextEdit(nullptr)
    , m_descriptionLineEdit(nullptr)
    , m_restoreAction(nullptr)
    , m_lockAction(nullptr)
    , m_isLocked(false)
{
    m_layout->setContentsMargins(QMargins());
    setLayout(m_layout);
    setupUi();
    updateValue();
}

/*!
 * \brief Assigns the specified \a tags and sets the specified \a fields using the given \a previousValueHandling.
 *
 * If \a preventUiUpdate is true, the UI will not be updated.
 */
void TagFieldEdit::setTagField(
    const QList<Tag *> &tags, TagParser::KnownField field, PreviousValueHandling previousValueHandling, bool preventUiUpdate)
{
    bool uiRebuildingRequired = false;
    m_tags = &tags;
    if (m_field != field) {
        m_field = field;
        uiRebuildingRequired = true;
    }
    if (!tags.empty()) {
        const auto proposedDataType = determineDataType();
        if (proposedDataType != m_dataType) {
            m_dataType = proposedDataType;
            uiRebuildingRequired = true;
        }
    }
    if (!preventUiUpdate) {
        if (uiRebuildingRequired) {
            setupUi();
        }
        updateValue(previousValueHandling);
    }
}

/*!
 * \brief Returns the currently shown value.
 * \remarks
 * - The specified \a encoding is used to encode text values.
 * - Does not work for values of the type picture.
 */
TagValue TagFieldEdit::value(TagTextEncoding encoding, bool includeDescription) const
{
    TagValue value;
    switch (m_dataType) {
    case TagDataType::Text:
    case TagDataType::TimeSpan:
    case TagDataType::DateTime:
        switch (m_field) {
        case KnownField::Genre:
            if (m_comboBox) {
                value.assignText(Utility::qstringToString(m_comboBox->currentText(), encoding), encoding);
            }
            break;
        case KnownField::Lyrics:
            if (m_plainTextEdit) {
                value.assignText(Utility::qstringToString(m_plainTextEdit->toPlainText(), encoding), encoding);
            }
            break;
        default:
            if (m_lineEdit) {
                value.assignText(Utility::qstringToString(m_lineEdit->text(), encoding), encoding);
            }
        }
        break;
    case TagDataType::Integer:
        if (m_spinBoxes.first && m_spinBoxes.first->value()) {
            value.assignInteger(m_spinBoxes.first->value());
        }
        break;
    case TagDataType::PositionInSet:
        if (m_spinBoxes.first && m_spinBoxes.second) {
            value.assignPosition(PositionInSet(m_spinBoxes.first->value(), m_spinBoxes.second->value()));
        }
        break;
    case TagDataType::StandardGenreIndex:
        if (m_comboBox) {
            value.assignText(Utility::qstringToString(m_comboBox->currentText(), encoding), encoding);
        }
        break;
    default:;
    }
    // setup description line edit
    if (m_descriptionLineEdit && m_descriptionLineEdit->isEnabled() && includeDescription) {
        value.setDescription(Utility::qstringToString(m_descriptionLineEdit->text(), encoding), encoding);
    }
    return value;
}

/*!
 * \brief Sets the \a value of the current tag field manually using the given \a previousValueHandling.
 *
 * Used for editing tags programmatically, eg. in TagEditorWidget::insertTitleFromFilename() and DbQueryWidget::applyResults().
 */
bool TagFieldEdit::setValue(const TagValue &value, PreviousValueHandling previousValueHandling)
{
    bool updated = updateValue(value, previousValueHandling, false);
    if (m_pictureSelection) {
        updated = m_pictureSelection->setValue(value, previousValueHandling) || updated;
    }
    return updated;
}

/*!
 * \brief Returns whether at least one of the assigned tags supports a description for the current field.
 */
bool TagFieldEdit::hasDescription() const
{
    for (const Tag *const tag : tags()) {
        if (tag->supportsDescription(m_field)) {
            return true;
        }
    }
    return false;
}

/*!
 * \brief Returns an indication whether the specified \a field can be applied.
 */
bool TagFieldEdit::canApply(KnownField field) const
{
    for (const Tag *const tag : tags()) {
        switch (tag->type()) {
        case TagType::Id3v1Tag:
            if (Settings::values().tagPocessing.creationSettings.id3v1usage == TagUsage::Never) {
                continue;
            }
            break;
        case TagType::Id3v2Tag:
            if (Settings::values().tagPocessing.creationSettings.id3v2usage == TagUsage::Never) {
                continue;
            }
            break;
        default:;
        }
        if (tag->supportsField(field)) {
            return true;
        }
    }
    return false;
}

/*!
 * \brief Sets whether the tag field edit is locked to keep the current value when switching files.
 */
void TagFieldEdit::setLocked(bool locked)
{
    if (locked == m_isLocked) {
        return;
    }
    m_isLocked = locked;
    if (m_lockAction) {
        m_lockAction->setIcon(QIcon::fromTheme(locked ? QStringLiteral("object-locked") : QStringLiteral("object-unlocked")));
        m_lockAction->setToolTip(
            locked ? tr("Keep previous value only if not present in the next file") : tr("Keep previous value even if present in next file"));
    }
}

/*!
 * \brief Sets whether the cover buttons are hidden.
 */
void TagFieldEdit::setCoverButtonsHidden(bool hideCoverButtons)
{
    if (m_pictureSelection) {
        m_pictureSelection->setCoverButtonsHidden(hideCoverButtons);
    }
}

/*!
 * \brief Internally called to determine the data type of the current tag field.
 */
TagDataType TagFieldEdit::determineDataType()
{
    TagDataType proposedDataType = TagDataType::Undefined;
    for (const Tag *const tag : tags()) {
        TagDataType type = tag->proposedDataType(m_field);
        if (proposedDataType == TagDataType::Undefined) {
            proposedDataType = type;
        } else if ((proposedDataType == TagDataType::PositionInSet && type == TagDataType::Integer)
            || (type == TagDataType::PositionInSet && proposedDataType == TagDataType::Integer)) {
            proposedDataType = TagDataType::PositionInSet; // PositionInSet and Number can be considered as compatible
        } else if (type == TagDataType::Undefined || type != proposedDataType) {
            return TagDataType::Undefined; // undefined or different (incompatible) types proposed
        }
    }
    return proposedDataType;
}

/*!
 * \brief Internally called to setup the UI.
 *
 * Causes the previous UI widgets to be destroyed. Any unapplied changes are lost.
 */
void TagFieldEdit::setupUi()
{
    // remove previous widgets
    m_lineEdit = nullptr;
    m_comboBox = nullptr;
    m_spinBoxes.first = nullptr;
    m_spinBoxes.second = nullptr;
    m_pictureSelection = nullptr;
    m_plainTextEdit = nullptr;
    m_descriptionLineEdit = nullptr;
    qDeleteAll(m_widgets);
    m_widgets.clear();
    // setup widgets
    switch (m_dataType) {
    case TagDataType::Text:
    case TagDataType::TimeSpan:
    case TagDataType::DateTime:
        switch (m_field) {
        case KnownField::Genre:
            setupGenreComboBox();
            break;
        case KnownField::Lyrics:
            setupPlainTextEdit();
            break;
        default:
            setupLineEdit();
        }
        break;
    case TagDataType::Picture:
        setupPictureSelection();
        break;
    case TagDataType::Integer:
        setupSpinBox();
        break;
    case TagDataType::PositionInSet:
        setupPositionInSetSpinBoxes();
        break;
    case TagDataType::StandardGenreIndex:
        setupGenreComboBox();
        break;
    case TagDataType::Binary:
        setupFileSelection();
        break;
    default:
        setupTypeNotSupportedLabel();
    }
    if (m_dataType != TagDataType::Picture && hasDescription()) { // setup description line edit
        setupDescriptionLineEdit();
    }
}

/*!
 * \brief Internally called by setupUi() to setup a line edit.
 */
ClearLineEdit *TagFieldEdit::setupLineEdit()
{
    m_lineEdit = new ClearLineEdit(this);
    m_lineEdit->setPlaceholderText(tr("empty"));
    static_cast<ButtonOverlay *>(m_lineEdit)->setClearButtonEnabled(true);
    m_lineEdit->addCustomAction(setupLockAction());
    m_lineEdit->addCustomAction(setupRestoreAction());
    m_lineEdit->installEventFilter(this);
    connect(m_lineEdit, &ClearLineEdit::textChanged, this, &TagFieldEdit::showRestoreAction);
    m_layout->addWidget(m_lineEdit);
    m_widgets << m_lineEdit;
    return m_lineEdit;
}

/*!
 * \brief Internally called by setupUi() to setup a plain text edit.
 */
ClearPlainTextEdit *TagFieldEdit::setupPlainTextEdit()
{
    m_plainTextEdit = new ClearPlainTextEdit(this);
    m_plainTextEdit->setTabChangesFocus(true);
    m_plainTextEdit->setClearButtonEnabled(true);
    m_plainTextEdit->addCustomAction(setupLockAction());
    m_plainTextEdit->addCustomAction(setupRestoreAction());
    m_plainTextEdit->setStyleSheet(
        QStringLiteral("color: ") + QGuiApplication::palette().text().color().name(QColor::HexArgb)); // not sure why this is otherwise gray
    connect(m_plainTextEdit->document(), &QTextDocument::contentsChanged, this, &TagFieldEdit::showRestoreAction);
    m_layout->addWidget(m_plainTextEdit);
    m_widgets << m_plainTextEdit;
    return m_plainTextEdit;
}

/*!
 * \brief Internally called by setupUi() to setup a combo box.
 */
ClearComboBox *TagFieldEdit::setupGenreComboBox()
{
    m_comboBox = new ClearComboBox(this);
    m_comboBox->setEditable(true);
    if (QLineEdit *lineEdit = m_comboBox->lineEdit()) {
        lineEdit->setPlaceholderText(tr("empty"));
    }
    m_comboBox->addItems(QStringList({ QString(), tr("Blues"), tr("A capella"), tr("Abstract"), tr("Acid"), tr("Acid Jazz"), tr("Acid Punk"),
        tr("Acoustic"), tr("Alternative"), tr("Alternative Rock"), tr("Ambient"), tr("Anime"), tr("Art Rock"), tr("Audio Theatre"), tr("Audiobook"),
        tr("Avantgarde"), tr("Ballad"), tr("Baroque"), tr("Bass"), tr("Beat"), tr("Bebop"), tr("Bhangra"), tr("Big Band"), tr("Big Beat"),
        tr("Black Metal"), tr("Bluegrass"), tr("Booty Bass"), tr("Breakbeat"), tr("BritPop"), tr("Cabaret"), tr("Celtic"), tr("Chamber Music"),
        tr("Chanson"), tr("Chillout"), tr("Chorus"), tr("Christian Gangsta Rap"), tr("Christian Rap"), tr("Christian Rock"), tr("Classic Rock"),
        tr("Classical"), tr("Club"), tr("Club-House"), tr("Comedy"), tr("Contemporary Christian"), tr("Country"), tr("Crossover"), tr("Cult"),
        tr("Dance"), tr("Dance Hall"), tr("Darkwave"), tr("Death Metal"), tr("Disco"), tr("Downtempo"), tr("Dream"), tr("Drum & Bass"),
        tr("Drum Solo"), tr("Dub"), tr("Dubstep"), tr("Duet"), tr("Easy Listening"), tr("EBM"), tr("Eclectic"), tr("Electro"), tr("Electroclash"),
        tr("Electronic"), tr("Emo"), tr("Ethnic"), tr("Euro-House"), tr("Euro-Techno"), tr("Eurodance"), tr("Experimental"), tr("Fast Fusion"),
        tr("Folk"), tr("Folk-Rock"), tr("Folklore"), tr("Freestyle"), tr("Funk"), tr("Fusion"), tr("G-Funk"), tr("Game"), tr("Gangsta"), tr("Garage"),
        tr("Garage Rock"), tr("Global"), tr("Goa"), tr("Gospel"), tr("Gothic"), tr("Gothic Rock"), tr("Grunge"), tr("Hard Rock"),
        tr("Hardcore Techno"), tr("Heavy Metal"), tr("Hip-Hop"), tr("House"), tr("Humour"), tr("IDM"), tr("Illbient"), tr("Indie"), tr("Indie Rock"),
        tr("Industrial"), tr("Industro-Goth"), tr("Instrumental"), tr("Instrumental Pop"), tr("Instrumental Rock"), tr("Jam Band"), tr("Jazz"),
        tr("Jazz & Funk"), tr("Jpop"), tr("Jungle"), tr("Krautrock"), tr("Latin"), tr("Leftfield"), tr("Lo-Fi"), tr("Lounge"), tr("Math Rock"),
        tr("Meditative"), tr("Merengue"), tr("Metal"), tr("Musical"), tr("National Folk"), tr("Native US"), tr("Negerpunk"), tr("Neoclassical"),
        tr("Neue Deutsche Welle"), tr("New Age"), tr("New Romantic"), tr("New Wave"), tr("Noise"), tr("Nu-Breakz"), tr("Oldies"), tr("Opera"),
        tr("Podcast"), tr("Polka"), tr("Polsk Punk"), tr("Pop"), tr("Pop-Folk"), tr("Pop/Funk"), tr("Porn Groove"), tr("Post-Punk"), tr("Post-Rock"),
        tr("Power Ballad"), tr("Pranks"), tr("Primus"), tr("Progressive Rock"), tr("Psychedelic"), tr("Psychedelic Rock"), tr("Psytrance"),
        tr("Punk"), tr("Punk Rock"), tr("Rap"), tr("Rave"), tr("Reggae"), tr("Retro"), tr("Revival"), tr("Rhythmic Soul"), tr("Rock"),
        tr("Rock & Roll"), tr("Salsa"), tr("Samba"), tr("Satire"), tr("Shoegaze"), tr("Showtunes"), tr("Ska"), tr("Slow Jam"), tr("Slow Rock"),
        tr("Sonata"), tr("Soul"), tr("Sound Clip"), tr("Soundtrack"), tr("Southern Rock"), tr("Space"), tr("Space Rock"), tr("Speech"), tr("Swing"),
        tr("Symphonic Rock"), tr("Symphony"), tr("Synthpop"), tr("Tango"), tr("Techno"), tr("Techno-Industrial"), tr("Terror"), tr("Thrash Metal"),
        tr("Top 40"), tr("Trailer"), tr("Trance"), tr("Tribal"), tr("Trip-Hop"), tr("Trop Rock"), tr("Vocal"), tr("World Music") }));
    m_comboBox->setCurrentIndex(0);
    m_comboBox->setClearButtonEnabled(true);
    m_comboBox->addCustomAction(setupLockAction());
    m_comboBox->addCustomAction(setupRestoreAction());
    m_comboBox->installEventFilter(this);
    m_comboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    connect(m_comboBox, &ClearComboBox::currentTextChanged, this, &TagFieldEdit::showRestoreAction);

    m_layout->addWidget(m_comboBox);
    m_widgets << m_comboBox;
    return m_comboBox;
}

/*!
 * \brief Internally called by setupUi() to setup a single spin box.
 */
ClearSpinBox *TagFieldEdit::setupSpinBox()
{
    m_spinBoxes.first = new ClearSpinBox(this);
    m_spinBoxes.first->setPlaceholderText(tr("empty"));
    m_spinBoxes.first->setMinimumHidden(true);
    m_spinBoxes.first->setClearButtonEnabled(true);
    m_spinBoxes.first->addCustomAction(setupLockAction());
    m_spinBoxes.first->addCustomAction(setupRestoreAction());
    m_spinBoxes.first->installEventFilter(this);
    m_spinBoxes.first->setMaximum(32766);
    m_spinBoxes.first->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    connect(m_spinBoxes.first, static_cast<void (ClearSpinBox::*)(int)>(&ClearSpinBox::valueChanged), this, &TagFieldEdit::showRestoreAction);
    m_layout->addWidget(m_spinBoxes.first);
    m_widgets << m_spinBoxes.first;
    return m_spinBoxes.first;
}

/*!
 * \brief Internally called by setupUi() to setup a a pair of spin boxes.
 */
QPair<QtUtilities::ClearSpinBox *, QtUtilities::ClearSpinBox *> &TagFieldEdit::setupPositionInSetSpinBoxes()
{
    auto *const subLayout = new QHBoxLayout;

    m_spinBoxes.first = new ClearSpinBox(this);
    m_spinBoxes.first->setPlaceholderText(tr("empty"));
    m_spinBoxes.first->setMinimumHidden(true);
    m_spinBoxes.first->setClearButtonEnabled(true);
    m_spinBoxes.first->installEventFilter(this);
    m_spinBoxes.first->setMaximum(32766);
    m_spinBoxes.first->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    connect(m_spinBoxes.first, static_cast<void (ClearSpinBox::*)(int)>(&ClearSpinBox::valueChanged), this, &TagFieldEdit::showRestoreAction);
    subLayout->addWidget(m_spinBoxes.first);
    m_widgets << m_spinBoxes.first;

    auto *const label = new QLabel(tr("of"), this);
    subLayout->addWidget(label);
    m_widgets << label;

    m_spinBoxes.second = new ClearSpinBox(this);
    m_spinBoxes.second->setPlaceholderText(tr("empty"));
    m_spinBoxes.second->setMinimumHidden(true);
    m_spinBoxes.second->setClearButtonEnabled(true);
    m_spinBoxes.second->installEventFilter(this);
    m_spinBoxes.second->setMaximum(32766);
    m_spinBoxes.second->addCustomAction(setupLockAction());
    m_spinBoxes.second->addCustomAction(setupRestoreAction());
    m_spinBoxes.second->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    connect(m_spinBoxes.second, static_cast<void (ClearSpinBox::*)(int)>(&ClearSpinBox::valueChanged), this, &TagFieldEdit::showRestoreAction);
    subLayout->addWidget(m_spinBoxes.second);
    m_widgets << m_spinBoxes.second;

    m_layout->addLayout(subLayout);
    return m_spinBoxes;
}

/*!
 * \brief Internally called by setupUi() to setup a PicturePreviewSelection widget.
 */
PicturePreviewSelection *TagFieldEdit::setupPictureSelection()
{
    m_pictureSelection = new PicturePreviewSelection(nullptr, KnownField::Invalid, this);
    connect(m_pictureSelection, &PicturePreviewSelection::pictureChanged, this, &TagFieldEdit::showRestoreAction);
    m_layout->addWidget(m_pictureSelection);
    m_widgets << m_pictureSelection;
    return m_pictureSelection;
}

/*!
 * \brief Internally called by setupUi() to setup a file selection.
 */
QWidget *TagFieldEdit::setupFileSelection()
{
    return setupTypeNotSupportedLabel();
}

/*!
 * \brief Internally called by setupUi() to setup a line edit for the description.
 */
ClearLineEdit *TagFieldEdit::setupDescriptionLineEdit()
{
    auto *const label = new QLabel(tr("Description"), this);
    m_layout->addWidget(label);

    m_widgets << label;
    m_descriptionLineEdit = new ClearLineEdit(this);
    m_descriptionLineEdit->setPlaceholderText(tr("empty"));
    static_cast<ButtonOverlay *>(m_descriptionLineEdit)->setClearButtonEnabled(true);
    m_descriptionLineEdit->installEventFilter(this);
    connect(m_descriptionLineEdit, &ClearLineEdit::textChanged, this, &TagFieldEdit::showRestoreAction);

    m_layout->addWidget(m_descriptionLineEdit);
    m_widgets << m_descriptionLineEdit;
    return m_descriptionLineEdit;
}

/*!
 * \brief Internally called by setupUi() to setup a label indicating that the current tag field is not supported by the edit.
 */
QLabel *TagFieldEdit::setupTypeNotSupportedLabel()
{
    auto *const label = new QLabel(tr("no widget for editing this field type available"), this);
    m_layout->addWidget(label);
    m_widgets << label;
    return label;
}

/*!
 * \brief Updates the currently shown value manually.
 * \param previousValueHandling Specifies how to deal with the previous value.
 *
 * The new value is read from the assigned tag(s).
 */
bool TagFieldEdit::updateValue(PreviousValueHandling previousValueHandling)
{
    // use the values from the last tag which has the specified field
    for (auto i = tags().crbegin(), end = tags().crend(); i != end; ++i) {
        // FIXME: use tag->values(m_field) and handle all values
        auto *const tag = *i;
        const TagValue &value = tag->value(m_field);
        if (value.isEmpty()) {
            continue;
        }

        bool updated = updateValue(value, previousValueHandling);
        if (m_pictureSelection) {
            updated = m_pictureSelection->setTagField(tag, m_field, previousValueHandling) || updated;
        }
        return updated;
    }

    // set an empty value
    bool updated = updateValue(TagValue(), previousValueHandling);
    if (m_pictureSelection) {
        // pass the last tag if present so the picture selection can operate on that tag instance and won't be disabled
        updated = m_pictureSelection->setTagField(m_tags->isEmpty() ? nullptr : m_tags->back(), m_field, previousValueHandling) || updated;
    }
    return updated;
}

/*!
 * \brief Updates the currently shown value manually.
 * \param tag Specifies the tag to read the new value from.
 * \param previousValueHandling Specifies how to deal with the previous value.
 * \remarks If \a tag is nullptr, the new value is empty.
 */
bool TagFieldEdit::updateValue(Tag *tag, PreviousValueHandling previousValueHandling)
{
    bool updated = updateValue(tag ? tag->value(m_field) : TagValue::empty(), previousValueHandling);
    if (m_pictureSelection) {
        updated = m_pictureSelection->setTagField(tag, m_field, previousValueHandling) || updated;
    }
    return updated;
}

/*!
 * \brief Updates the currently shown value manually.
 * \param value Specifies the new value.
 * \param previousValueHandling Specifies how to deal with the previous value.
 * \param updateRestoreButton Specifies whether the "restore button" should be updated.
 * \remarks Does not update the picture preview selection.
 */
bool TagFieldEdit::updateValue(const TagValue &value, PreviousValueHandling previousValueHandling, bool updateRestoreButton)
{
    bool autoCorrectionApplied = false;
    bool conversionError = false;
    bool updated = false;
    concretizePreviousValueHandling(previousValueHandling);

    // setup widget for editing value
    if (m_lineEdit || m_comboBox || m_plainTextEdit) {
        const auto text([&] {
            try {
                const auto textValue = Utility::tagValueToQString(value);
                const auto correctedText = applyAutoCorrection(textValue);
                if (correctedText != textValue) {
                    autoCorrectionApplied = true;
                }
                return correctedText;
            } catch (const ConversionException &) {
                conversionError = true;
                return QString();
            }
        }());
        if ((!m_isLocked || text.isEmpty()) && (previousValueHandling == PreviousValueHandling::Clear || !text.isEmpty())) {
            if (m_lineEdit && (previousValueHandling != PreviousValueHandling::Keep || m_lineEdit->isCleared())) {
                m_lineEdit->setText(text);
                updated = true;
            }
            if (m_comboBox && (previousValueHandling != PreviousValueHandling::Keep || m_comboBox->isCleared())) {
                m_comboBox->setCurrentText(text);
                updated = true;
            }
            if (m_plainTextEdit && (previousValueHandling != PreviousValueHandling::Keep || m_plainTextEdit->isCleared())) {
                m_plainTextEdit->setPlainText(text);
                updated = true;
            }
        }
    }
    if (m_spinBoxes.first) {
        if (m_spinBoxes.second) {
            const auto pos([&] {
                try {
                    return value.toPositionInSet();
                } catch (const ConversionException &) {
                    conversionError = true;
                    return PositionInSet();
                }
            }());
            if (!m_isLocked || !pos.position()) {
                if (previousValueHandling == PreviousValueHandling::Clear || pos.position()) {
                    if (previousValueHandling != PreviousValueHandling::Keep || m_spinBoxes.first->isCleared()) {
                        m_spinBoxes.first->setValue(pos.position());
                        updated = true;
                    }
                } else if (previousValueHandling == PreviousValueHandling::IncrementUpdate && !m_spinBoxes.first->isCleared()) {
                    m_spinBoxes.first->setValue(m_spinBoxes.first->value() + 1);
                    updated = true;
                }
            }
            if (!m_isLocked || !pos.total()) {
                if (previousValueHandling == PreviousValueHandling::Clear || pos.total()) {
                    if (previousValueHandling != PreviousValueHandling::Keep || m_spinBoxes.second->isCleared()) {
                        m_spinBoxes.second->setValue(pos.total());
                        updated = true;
                    }
                }
            }
        } else {
            const auto num([&] {
                try {
                    return value.toInteger();
                } catch (const ConversionException &) {
                    conversionError = true;
                    return 0;
                }
            }());
            if (!m_isLocked || !num) {
                if (previousValueHandling == PreviousValueHandling::Clear || num) {
                    if (previousValueHandling != PreviousValueHandling::Keep || m_spinBoxes.first->isCleared()) {
                        m_spinBoxes.first->setValue(num);
                        updated = true;
                    }
                } else if (previousValueHandling == PreviousValueHandling::IncrementUpdate && !m_spinBoxes.first->isCleared()) {
                    m_spinBoxes.first->setValue(m_spinBoxes.first->value() + 1);
                    updated = true;
                }
            }
        }
    }

    // setup description line edit
    const auto shouldHaveDescriptionLineEdit = m_dataType != TagDataType::Picture && hasDescription();
    if (shouldHaveDescriptionLineEdit) {
        if (!m_descriptionLineEdit) {
            setupDescriptionLineEdit();
        }
        m_descriptionLineEdit->setEnabled(true);
        if (previousValueHandling != PreviousValueHandling::Keep || m_descriptionLineEdit->isCleared()) {
            try {
                const auto desc = Utility::stringToQString(value.description(), value.descriptionEncoding());
                const auto correctedDesc = applyAutoCorrection(desc);
                if (correctedDesc != desc) {
                    autoCorrectionApplied = true;
                }
                if (!m_isLocked || correctedDesc.isEmpty()) {
                    m_descriptionLineEdit->setText(correctedDesc);
                }
            } catch (const ConversionException &) {
                conversionError = true;
                if (!m_isLocked) {
                    m_descriptionLineEdit->clear();
                }
            }
        }
    } else if (m_descriptionLineEdit) {
        m_descriptionLineEdit->setEnabled(false);
        m_descriptionLineEdit->clear();
    }
    if (updateRestoreButton && m_restoreAction) {
        m_restoreAction->setVisible((!updated && m_restoreAction->isVisible()) || m_tags->size() > 1);
    }
    if (updated) {
        setLocked(false);
    }
    m_autoCorrectionApplied = updated && autoCorrectionApplied;

    // setup info button
    const auto widgets = initializer_list<ButtonOverlay *>{ m_lineEdit, m_comboBox, m_spinBoxes.first, m_spinBoxes.second };
    const auto canApplyField = canApply(m_field);
    if (!conversionError && canApplyField) {
        for (auto *const overlay : widgets) {
            if (overlay) {
                overlay->disableInfoButton();
            }
        }
        return updated;
    }
    const auto pixmap(QIcon::fromTheme(QStringLiteral("emblem-error")).pixmap(16));
    const auto text([&] {
        QString textValue;
        if (conversionError) {
            textValue = tr("The value of this field could not be read from the file because it couldn't be converted properly.");
            if (!canApplyField) {
                textValue += QChar('\n');
            }
        }
        if (!canApplyField) {
            textValue += tr("The field can not be applied when saving the file and will be lost.");
        }
        return textValue;
    }());
    for (auto *const overlay : widgets) {
        if (overlay) {
            overlay->enableInfoButton(pixmap, text);
        }
    }
    return updated;
}

/*!
 * \brief Internally called by the other setup methods to create the "restore button".
 */
QAction *TagFieldEdit::setupRestoreAction()
{
    if (m_restoreAction) {
        return m_restoreAction;
    }
    m_restoreAction = new QAction(this);
    m_restoreAction->setIcon(QIcon::fromTheme(QStringLiteral("edit-undo")));
    m_restoreAction->setToolTip(tr("Restore value as it is currently present in the file"));
    connect(m_restoreAction, &QAction::triggered, this, &TagFieldEdit::handleRestoreButtonClicked);
    // ownership might be transferred to a child widget/layout
    connect(m_restoreAction, &QAction::destroyed, this, &TagFieldEdit::handleRestoreButtonDestroyed);
    return m_restoreAction;
}

/*!
 * \brief Internally called by the other setup methods to create the "lock button".
 */
QAction *TagFieldEdit::setupLockAction()
{
    if (m_lockAction) {
        return m_lockAction;
    }
    m_isLocked = !m_isLocked;
    m_lockAction = new QAction(this);
    setLocked(!m_isLocked);
    connect(m_lockAction, &QAction::triggered, this, &TagFieldEdit::toggleLocked);
    // ownership might be transferred to a child widget/layout
    connect(m_lockAction, &QAction::destroyed, this, &TagFieldEdit::handleLockButtonDestroyed);
    return m_lockAction;
}

/*!
 * \brief Internally called to show the restore button (if there is one and at least one tag is assigned).
 */
void TagFieldEdit::showRestoreAction()
{
    if (m_restoreAction) {
        m_restoreAction->setVisible(m_tags->size());
    }
}

/*!
 * \brief Applies auto correction (according to the settings) for the specified \a textValue.
 */
QString TagFieldEdit::applyAutoCorrection(const QString &textValue)
{
    QString correctedValue = textValue;
    const auto &settings = Settings::values().editor.autoCompletition;
    auto &fields = settings.fields.items();
    auto i = find_if(fields.constBegin(), fields.constEnd(), [this](const ChecklistItem &item) {
        bool ok;
        return (item.id().toInt(&ok) == static_cast<int>(this->field())) && ok;
    });
    // if current field is in the list of auto correction fields and auto correction should be applied
    if (i == fields.constEnd() || !i->isChecked()) {
        return correctedValue;
    }
    if (settings.trimWhitespaces) {
        correctedValue = correctedValue.trimmed();
    }
    if (settings.formatNames) {
        correctedValue = Utility::formatName(correctedValue);
    }
    if (settings.fixUmlauts) {
        correctedValue = Utility::fixUmlauts(correctedValue);
    }
    const auto &subst = settings.customSubstitution;
    if (subst.enabled && subst.regex.isValid()) {
        correctedValue.replace(subst.regex, subst.replacement);
    }
    return correctedValue;
}

/*!
 * \brief If \a previousValueHandling is PreviousValueHandling::Auto it will be set to a concrete value.
 */
void TagFieldEdit::concretizePreviousValueHandling(PreviousValueHandling &previousValueHandling)
{
    switch (previousValueHandling) {
    case PreviousValueHandling::Auto:
        switch (m_field) {
        // these differ for each song -> always clear previous value
        case KnownField::Title:
        case KnownField::Lyrics:
            previousValueHandling = PreviousValueHandling::Clear;
            break;
        // these will be incremented
        case KnownField::TrackPosition:
        case KnownField::PartNumber:
            previousValueHandling = PreviousValueHandling::IncrementUpdate;
            break;
        default:
            previousValueHandling = PreviousValueHandling::Update;
        }
        break;
    default:;
    }
}

/*!
 * \brief Restores the current value to the initial value.
 */
void TagFieldEdit::restore()
{
    updateValue(PreviousValueHandling::Clear);
}

/*!
 * \brief Clears the current value.
 */
void TagFieldEdit::clear()
{
    if (m_lineEdit) {
        m_lineEdit->clear();
    }
    if (m_comboBox) {
        m_comboBox->setCurrentText(QString());
    }
    if (m_plainTextEdit) {
        m_plainTextEdit->clear();
    }
    if (m_spinBoxes.first) {
        m_spinBoxes.first->setValue(0);
    }
    if (m_spinBoxes.second) {
        m_spinBoxes.second->setValue(0);
    }
    if (m_pictureSelection) {
        m_pictureSelection->clear();
    }
    if (m_descriptionLineEdit) {
        m_descriptionLineEdit->clear();
    }
}

/*!
 * \brief Applies the current value to the assigned tag.
 */
void TagFieldEdit::apply()
{
    for (Tag *const tag : *m_tags) {
        if (m_dataType == TagDataType::Picture) {
            if (m_pictureSelection) {
                m_pictureSelection->apply();
            }
        } else {
            TagTextEncoding encoding = Settings::values().tagPocessing.preferredEncoding;
            if (!tag->canEncodingBeUsed(encoding)) {
                encoding = tag->proposedTextEncoding();
            }
            // FIXME: use tag->setValues(...) and to set multiple values
            tag->setValue(m_field, value(encoding, tag->supportsDescription(m_field)));
        }
    }
}

/*!
 * \brief Handles different key events.
 */
bool TagFieldEdit::eventFilter(QObject *obj, QEvent *event)
{
    switch (event->type()) {
    case QEvent::KeyRelease: {
        auto *const keyEvent = static_cast<QKeyEvent *>(event);
        switch (keyEvent->key()) {
        case Qt::Key_Return:
            emit returnPressed();
            break;
        case Qt::Key_Shift:
            if (keyEvent->modifiers() & Qt::ControlModifier) {
                if (auto *const le = qobject_cast<QLineEdit *>(obj)) {
                    le->setText(Utility::formatName(le->text()));
                    return true;
                }
            }
            break;
        default:;
        }
        break;
    }
    default:;
    }
    return QWidget::eventFilter(obj, event);
}

/*!
 * \brief Called when the restore button has been clicked.
 */
void TagFieldEdit::handleRestoreButtonClicked()
{
    if (tags().empty()) {
        restore();
        return;
    }
    QMenu menu;
    int i = 0;
    for (auto *const tag : tags()) {
        const auto *const action = menu.addAction(tr("restore to value from %1 (%2)").arg(qstringFromStdStringView(tag->typeName())).arg(++i));
        connect(action, &QAction::triggered, [this, tag] {
            setLocked(false);
            updateValue(tag, PreviousValueHandling::Clear);
        });
    }
    menu.exec(QCursor::pos());
}

/*!
 * \brief Sets m_restoreButton to nullptr when the restore button has been destroyed.
 */
void TagFieldEdit::handleRestoreButtonDestroyed(QObject *obj)
{
    if (obj == m_restoreAction) {
        m_restoreAction = nullptr;
    }
}

/*!
 * \brief Sets m_lockButton to nullptr when the restore button has been destroyed.
 */
void TagFieldEdit::handleLockButtonDestroyed(QObject *obj)
{
    if (obj == m_lockAction) {
        m_lockAction = nullptr;
    }
}

} // namespace QtGui
