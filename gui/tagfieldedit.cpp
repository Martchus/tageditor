#include "./tagfieldedit.h"
#include "./picturepreviewselection.h"

#include "../application/settings.h"
#include "../application/knownfieldmodel.h"

#include "../misc/utility.h"

#include <tagparser/tag.h>
#include <tagparser/tagvalue.h>
#include <tagparser/id3/id3v2tag.h>
#include <tagparser/mediafileinfo.h>

#include <qtutilities/widgets/clearlineedit.h>
#include <qtutilities/widgets/clearcombobox.h>
#include <qtutilities/widgets/clearspinbox.h>
#include <qtutilities/widgets/clearplaintextedit.h>
#include <qtutilities/widgets/iconbutton.h>

#include <c++utilities/conversion/conversionexception.h>

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QFile>
#include <QImage>
#include <QEvent>
#include <QKeyEvent>
#include <QListIterator>
#include <QCursor>
#include <QMenu>
#include <QAction>

#include <initializer_list>
#include <algorithm>
#include <functional>
#include <iostream>

using namespace std;
using namespace Models;
using namespace Widgets;
using namespace Media;
using namespace ConversionUtilities;

namespace QtGui {

/*!
 * \class QtGui::TagFieldEdit
 * \brief The TagFieldEdit widget allows the user to edit a specified tag field.
 */

/*!
 * \brief Constructs a new TagFieldEdit.
 * \sa setTagField()
 */
TagFieldEdit::TagFieldEdit(const QList<Media::Tag *> &tags, Media::KnownField field, QWidget *parent) :
    QWidget(parent),
    m_layout(new QVBoxLayout(this)),
    m_tags(&tags),
    m_field(field),
    m_dataType(determineDataType()),
    m_lineEdit(nullptr),
    m_comboBox(nullptr),
    m_spinBoxes(QPair<Widgets::ClearSpinBox *, Widgets::ClearSpinBox *>(nullptr, nullptr)),
    m_pictureSelection(nullptr),
    m_plainTextEdit(nullptr),
    m_descriptionLineEdit(nullptr),
    m_restoreButton(nullptr)
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
void TagFieldEdit::setTagField(const QList<Tag *> &tags, Media::KnownField field, PreviousValueHandling previousValueHandling, bool preventUiUpdate)
{
    bool uiRebuildingRequired = false;
    m_tags = &tags;
    if(m_field != field) {
        m_field = field;
        uiRebuildingRequired = true;
    }
    if(tags.size()) {
        TagDataType proposedDataType = determineDataType();
        if(proposedDataType != m_dataType) {
            m_dataType = proposedDataType;
            uiRebuildingRequired = true;
        }
    }
    if(!preventUiUpdate) {
        if(uiRebuildingRequired) {
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
    switch(m_dataType) {
    case TagDataType::Text:
    case TagDataType::TimeSpan:
    case TagDataType::DateTime:
        switch(m_field) {
        case KnownField::Genre:
            if(m_comboBox) {
                value.assignText(Utility::qstringToString(m_comboBox->currentText(), encoding), encoding);
            }
            break;
        case KnownField::Lyrics:
            if(m_plainTextEdit) {
                value.assignText(Utility::qstringToString(m_plainTextEdit->toPlainText(), encoding), encoding);
            }
            break;
        default:
            if(m_lineEdit) {
                value.assignText(Utility::qstringToString(m_lineEdit->text(), encoding), encoding);
            }
        }
        break;
    case TagDataType::Integer:
        if(m_spinBoxes.first && m_spinBoxes.first->value()) {
            value.assignInteger(m_spinBoxes.first->value());
        }
        break;
    case TagDataType::PositionInSet:
        if(m_spinBoxes.first && m_spinBoxes.second) {
            value.assignPosition(PositionInSet(m_spinBoxes.first->value(), m_spinBoxes.second->value()));
        }
        break;
    case TagDataType::StandardGenreIndex:
        if(m_comboBox) {
            value.assignText(Utility::qstringToString(m_comboBox->currentText(), encoding), encoding);
        }
        break;
    default:
        ;
    }
    if(m_descriptionLineEdit && includeDescription) { // setup description line edit
        value.setDescription(Utility::qstringToString(m_descriptionLineEdit->text(), encoding), encoding);
    }
    return value;
}

/*!
 * \brief Sets the \a value of the current tag field manually using the given \a previousValueHandling.
 */
bool TagFieldEdit::setValue(const TagValue &value, PreviousValueHandling previousValueHandling)
{
    updateValue(value, previousValueHandling, false);
    return m_pictureSelection == nullptr;
}

/*!
 * \brief Returns whether at least one of the assigned tags supports a description for the current field.
 */
bool TagFieldEdit::hasDescription() const
{
    foreach(Tag *tag, tags()) {
        if(tag->supportsDescription(m_field)) {
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
    foreach(Tag *tag, tags()) {
        switch(tag->type()) {
        case TagType::Id3v1Tag:
            if(Settings::id3v1usage() == TagUsage::Never) {
                continue;
            }
            break;
        case TagType::Id3v2Tag:
            if(Settings::id3v2usage() == TagUsage::Never) {
                continue;
            }
            break;
        default:
            ;
        }
        if(tag->supportsField(field)) {
            return true;
        }
    }
    return false;
}

/*!
 * \brief Internally called to determine the data type of the current tag field.
 */
TagDataType TagFieldEdit::determineDataType()
{
    TagDataType proposedDataType = TagDataType::Undefined;
    foreach(Tag *tag, tags()) {
        TagDataType type = tag->proposedDataType(m_field);
        if(proposedDataType == TagDataType::Undefined) {
            proposedDataType = type;
        } else if((proposedDataType == TagDataType::PositionInSet && type == TagDataType::Integer)
                  || (type == TagDataType::PositionInSet && proposedDataType == TagDataType::Integer)) {
            proposedDataType = TagDataType::PositionInSet; // PositionInSet and Number can be considered as compatible
        } else if(type == TagDataType::Undefined || type != proposedDataType) {
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
    switch(m_dataType) {
    case TagDataType::Text:
    case TagDataType::TimeSpan:
    case TagDataType::DateTime:
        switch(m_field) {
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
        ;
    }
    if(m_dataType != TagDataType::Picture
            && hasDescription()) { // setup description line edit
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
    m_lineEdit->insertCustomButton(0, setupRestoreButton());
    m_lineEdit->installEventFilter(this);
    connect(m_lineEdit, &ClearLineEdit::textChanged, this, &TagFieldEdit::showRestoreButton);
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
    m_plainTextEdit->setClearButtonEnabled(true);
    m_plainTextEdit->insertCustomButton(0, setupRestoreButton());
    connect(m_plainTextEdit->document(), &QTextDocument::contentsChanged, this, &TagFieldEdit::showRestoreButton);
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
    if(QLineEdit *lineEdit = m_comboBox->lineEdit()) {
        lineEdit->setPlaceholderText(tr("empty"));
    }
    m_comboBox->addItems(QStringList()
                         << QString()
                         << tr("Blues")
                         << tr("A capella")
                         << tr("Abstract")
                         << tr("Acid")
                         << tr("Acid Jazz")
                         << tr("Acid Punk")
                         << tr("Acoustic")
                         << tr("Alternative")
                         << tr("Alternative Rock")
                         << tr("Ambient")
                         << tr("Anime")
                         << tr("Art Rock")
                         << tr("Audio Theatre")
                         << tr("Audiobook")
                         << tr("Avantgarde")
                         << tr("Ballad")
                         << tr("Baroque")
                         << tr("Bass")
                         << tr("Beat")
                         << tr("Bebop")
                         << tr("Bhangra")
                         << tr("Big Band")
                         << tr("Big Beat")
                         << tr("Black Metal")
                         << tr("Bluegrass")
                         << tr("Booty Bass")
                         << tr("Breakbeat")
                         << tr("BritPop")
                         << tr("Cabaret")
                         << tr("Celtic")
                         << tr("Chamber Music")
                         << tr("Chanson")
                         << tr("Chillout")
                         << tr("Chorus")
                         << tr("Christian Gangsta Rap")
                         << tr("Christian Rap")
                         << tr("Christian Rock")
                         << tr("Classic Rock")
                         << tr("Classical")
                         << tr("Club")
                         << tr("Club-House")
                         << tr("Comedy")
                         << tr("Contemporary Christian")
                         << tr("Country")
                         << tr("Crossover")
                         << tr("Cult")
                         << tr("Dance")
                         << tr("Dance Hall")
                         << tr("Darkwave")
                         << tr("Death Metal")
                         << tr("Disco")
                         << tr("Downtempo")
                         << tr("Dream")
                         << tr("Drum & Bass")
                         << tr("Drum Solo")
                         << tr("Dub")
                         << tr("Dubstep")
                         << tr("Duet")
                         << tr("Easy Listening")
                         << tr("EBM")
                         << tr("Eclectic")
                         << tr("Electro")
                         << tr("Electroclash")
                         << tr("Electronic")
                         << tr("Emo")
                         << tr("Ethnic")
                         << tr("Euro-House")
                         << tr("Euro-Techno")
                         << tr("Eurodance")
                         << tr("Experimental")
                         << tr("Fast Fusion")
                         << tr("Folk")
                         << tr("Folk-Rock")
                         << tr("Folklore")
                         << tr("Freestyle")
                         << tr("Funk")
                         << tr("Fusion")
                         << tr("G-Funk")
                         << tr("Game")
                         << tr("Gangsta")
                         << tr("Garage")
                         << tr("Garage Rock")
                         << tr("Global")
                         << tr("Goa")
                         << tr("Gospel")
                         << tr("Gothic")
                         << tr("Gothic Rock")
                         << tr("Grunge")
                         << tr("Hard Rock")
                         << tr("Hardcore Techno")
                         << tr("Heavy Metal")
                         << tr("Hip-Hop")
                         << tr("House")
                         << tr("Humour")
                         << tr("IDM")
                         << tr("Illbient")
                         << tr("Indie")
                         << tr("Indie Rock")
                         << tr("Industrial")
                         << tr("Industro-Goth")
                         << tr("Instrumental")
                         << tr("Instrumental Pop")
                         << tr("Instrumental Rock")
                         << tr("Jam Band")
                         << tr("Jazz")
                         << tr("Jazz & Funk")
                         << tr("Jpop")
                         << tr("Jungle")
                         << tr("Krautrock")
                         << tr("Latin")
                         << tr("Leftfield")
                         << tr("Lo-Fi")
                         << tr("Lounge")
                         << tr("Math Rock")
                         << tr("Meditative")
                         << tr("Merengue")
                         << tr("Metal")
                         << tr("Musical")
                         << tr("National Folk")
                         << tr("Native US")
                         << tr("Negerpunk")
                         << tr("Neoclassical")
                         << tr("Neue Deutsche Welle")
                         << tr("New Age")
                         << tr("New Romantic")
                         << tr("New Wave")
                         << tr("Noise")
                         << tr("Nu-Breakz")
                         << tr("Oldies")
                         << tr("Opera")
                         << tr("Podcast")
                         << tr("Polka")
                         << tr("Polsk Punk")
                         << tr("Pop")
                         << tr("Pop-Folk")
                         << tr("Pop/Funk")
                         << tr("Porn Groove")
                         << tr("Post-Punk")
                         << tr("Post-Rock")
                         << tr("Power Ballad")
                         << tr("Pranks")
                         << tr("Primus")
                         << tr("Progressive Rock")
                         << tr("Psychedelic")
                         << tr("Psychedelic Rock")
                         << tr("Psytrance")
                         << tr("Punk")
                         << tr("Punk Rock")
                         << tr("Rap")
                         << tr("Rave")
                         << tr("Reggae")
                         << tr("Retro")
                         << tr("Revival")
                         << tr("Rhythmic Soul")
                         << tr("Rock")
                         << tr("Rock & Roll")
                         << tr("Salsa")
                         << tr("Samba")
                         << tr("Satire")
                         << tr("Shoegaze")
                         << tr("Showtunes")
                         << tr("Ska")
                         << tr("Slow Jam")
                         << tr("Slow Rock")
                         << tr("Sonata")
                         << tr("Soul")
                         << tr("Sound Clip")
                         << tr("Soundtrack")
                         << tr("Southern Rock")
                         << tr("Space")
                         << tr("Space Rock")
                         << tr("Speech")
                         << tr("Swing")
                         << tr("Symphonic Rock")
                         << tr("Symphony")
                         << tr("Synthpop")
                         << tr("Tango")
                         << tr("Techno")
                         << tr("Techno-Industrial")
                         << tr("Terror")
                         << tr("Thrash Metal")
                         << tr("Top 40")
                         << tr("Trailer")
                         << tr("Trance")
                         << tr("Tribal")
                         << tr("Trip-Hop")
                         << tr("Trop Rock")
                         << tr("Vocal")
                         << tr("World Music"));
    m_comboBox->setCurrentIndex(0);
    m_comboBox->setClearButtonEnabled(true);
    m_comboBox->insertCustomButton(0, setupRestoreButton());
    m_comboBox->installEventFilter(this);
    connect(m_comboBox, &ClearComboBox::currentTextChanged, this, &TagFieldEdit::showRestoreButton);

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
    m_spinBoxes.first->insertCustomButton(0, setupRestoreButton());
    m_spinBoxes.first->installEventFilter(this);
    m_spinBoxes.first->setMaximum(32766);
    connect(m_spinBoxes.first, static_cast<void (ClearSpinBox::*)(int)>(&ClearSpinBox::valueChanged), this, &TagFieldEdit::showRestoreButton);
    m_layout->addWidget(m_spinBoxes.first);
    m_widgets << m_spinBoxes.first;
    return m_spinBoxes.first;
}

/*!
 * \brief Internally called by setupUi() to setup a a pair of spin boxes.
 */
QPair<Widgets::ClearSpinBox *, Widgets::ClearSpinBox *> &TagFieldEdit::setupPositionInSetSpinBoxes()
{
    QHBoxLayout *subLayout = new QHBoxLayout;

    m_spinBoxes.first = new ClearSpinBox(this);
    m_spinBoxes.first->setPlaceholderText(tr("empty"));
    m_spinBoxes.first->setMinimumHidden(true);
    m_spinBoxes.first->setClearButtonEnabled(true);
    m_spinBoxes.first->installEventFilter(this);
    m_spinBoxes.first->setMaximum(32766);
    m_spinBoxes.first->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    connect(m_spinBoxes.first, static_cast<void (ClearSpinBox::*)(int)>(&ClearSpinBox::valueChanged), this, &TagFieldEdit::showRestoreButton);
    subLayout->addWidget(m_spinBoxes.first);
    m_widgets << m_spinBoxes.first;

    QLabel *label = new QLabel(tr("of"), this);
    subLayout->addWidget(label);
    m_widgets << label;

    m_spinBoxes.second = new ClearSpinBox(this);
    m_spinBoxes.second->setPlaceholderText(tr("empty"));
    m_spinBoxes.second->setMinimumHidden(true);
    m_spinBoxes.second->setClearButtonEnabled(true);
    m_spinBoxes.second->installEventFilter(this);
    m_spinBoxes.second->setMaximum(32766);
    m_spinBoxes.second->insertCustomButton(0, setupRestoreButton());
    m_spinBoxes.second->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    connect(m_spinBoxes.second, static_cast<void (ClearSpinBox::*)(int)>(&ClearSpinBox::valueChanged), this, &TagFieldEdit::showRestoreButton);
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
    connect(m_pictureSelection, &PicturePreviewSelection::pictureChanged, this, &TagFieldEdit::showRestoreButton);
    m_layout->addWidget(m_pictureSelection);
    m_widgets << m_pictureSelection;
    return m_pictureSelection;
}

/*!
 * \brief Internally called by setupUi() to setup a file selection.
 * \remarks TODO
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
    QLabel *label = new QLabel(tr("Description"), this);
    m_layout->addWidget(label);

    m_widgets << label;
    m_descriptionLineEdit = new ClearLineEdit(this);
    m_descriptionLineEdit->setPlaceholderText(tr("empty"));
    static_cast<ButtonOverlay *>(m_descriptionLineEdit)->setClearButtonEnabled(true);
    m_descriptionLineEdit->installEventFilter(this);
    connect(m_descriptionLineEdit, &ClearLineEdit::textChanged, this, &TagFieldEdit::showRestoreButton);

    m_layout->addWidget(m_descriptionLineEdit);
    m_widgets << m_descriptionLineEdit;
    return m_descriptionLineEdit;
}

/*!
 * \brief Internally called by setupUi() to setup a label indicating that the current tag field is not supported by the edit.
 */
QLabel *TagFieldEdit::setupTypeNotSupportedLabel()
{
    QLabel *label = new QLabel(tr("editing widget for field type not supported"), this);
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
void TagFieldEdit::updateValue(PreviousValueHandling previousValueHandling)
{
    QListIterator<Tag *> i(tags());
    i.toBack();
    bool pictureSelectionUpdated = false;
    while(i.hasPrevious()) {
        Tag *tag = i.previous();
        const TagValue &value = tag->value(m_field);
        if(!value.isEmpty()) {
            updateValue(value, previousValueHandling);
            if(m_pictureSelection) {
                m_pictureSelection->setTagField(tag, m_field, previousValueHandling);
                pictureSelectionUpdated = true;
            }
            return;
        }
    }
    updateValue(TagValue::empty(), previousValueHandling);
    if(m_pictureSelection && !pictureSelectionUpdated) {
        if(m_tags->size()) {
            m_pictureSelection->setTagField(m_tags->last(), m_field, previousValueHandling);
        } else {
            m_pictureSelection->setTagField(nullptr, m_field, previousValueHandling);
        }
    }
}

/*!
 * \brief Updates the currently shown value manually.
 * \param tag Specifies the tag to read the new value from.
 * \param previousValueHandling Specifies how to deal with the previous value.
 * \remarks If \a tag is nullptr, the new value is empty.
 */
void TagFieldEdit::updateValue(Tag *tag, PreviousValueHandling previousValueHandling)
{
    if(tag) {
        updateValue(tag->value(m_field), previousValueHandling);
    } else {
        updateValue(TagValue::empty(), previousValueHandling);
    }
    if(m_pictureSelection) {
        m_pictureSelection->setTagField(tag, m_field, previousValueHandling);
    }
}

/*!
 * \brief Updates the currently shown value manually.
 * \param value Specifies the new value.
 * \param previousValueHandling Specifies how to deal with the previous value.
 * \param updateRestoreButton Specifies whether the "restore button" should be updated.
 */
void TagFieldEdit::updateValue(const TagValue &value, PreviousValueHandling previousValueHandling, bool updateRestoreButton)
{
    bool conversionError = false;
    bool updated = false;
    concretizePreviousValueHandling(previousValueHandling);
    if(m_lineEdit || m_comboBox || m_plainTextEdit) {
        QString text;
        try {
            text = Utility::tagValueToQString(value);
        } catch (ConversionException &) {
            conversionError = true;
        }
        applyAutoCorrection(text);
        if(previousValueHandling == PreviousValueHandling::Clear || !text.isEmpty()) {
            if(m_lineEdit && (previousValueHandling != PreviousValueHandling::Keep || m_lineEdit->isCleared())) {
                m_lineEdit->setText(text);
                updated = true;
            }
            if(m_comboBox && (previousValueHandling != PreviousValueHandling::Keep || m_comboBox->isCleared())) {
                m_comboBox->setCurrentText(text);
                updated = true;
            }
            if(m_plainTextEdit && (previousValueHandling != PreviousValueHandling::Keep || m_plainTextEdit->isCleared())) {
                m_plainTextEdit->setPlainText(text);
                updated = true;
            }
        }
    }
    if(m_spinBoxes.first) {
        if(m_spinBoxes.second) {
            PositionInSet pos;
            try {
                pos = value.toPositionIntSet();
            } catch(const ConversionException &) {
                conversionError = true;
            }
            if(previousValueHandling == PreviousValueHandling::Clear || pos.position()) {
                if(previousValueHandling != PreviousValueHandling::Keep || m_spinBoxes.first->isCleared()) {
                    m_spinBoxes.first->setValue(pos.position());
                    updated = true;
                }
            } else if(previousValueHandling == PreviousValueHandling::IncrementUpdate && !m_spinBoxes.first->isCleared()) {
                m_spinBoxes.first->setValue(m_spinBoxes.first->value() + 1);
                updated = true;
            }
            if(previousValueHandling == PreviousValueHandling::Clear || pos.total()) {
                if(previousValueHandling != PreviousValueHandling::Keep || m_spinBoxes.second->isCleared()) {
                    m_spinBoxes.second->setValue(pos.total());
                    updated = true;
                }
            }
        } else {
            int num;
            try {
                num = value.toInteger();
            } catch(const ConversionException &) {
                conversionError = true;
                num = 0;
            }
            if(previousValueHandling == PreviousValueHandling::Clear || num) {
                if(previousValueHandling != PreviousValueHandling::Keep || m_spinBoxes.first->isCleared()) {
                    m_spinBoxes.first->setValue(num);
                    updated = true;
                }
            } else if(previousValueHandling == PreviousValueHandling::IncrementUpdate && !m_spinBoxes.first->isCleared()) {
                m_spinBoxes.first->setValue(m_spinBoxes.first->value() + 1);
                updated = true;
            }
        }
    }
    if(m_descriptionLineEdit && (previousValueHandling != PreviousValueHandling::Keep || m_descriptionLineEdit->isCleared())) {
        try {
            QString desc = Utility::stringToQString(value.description(), value.descriptionEncoding());
            applyAutoCorrection(desc);
            m_descriptionLineEdit->setText(desc);
        } catch(ConversionException &) {
            conversionError = true;
            m_descriptionLineEdit->clear();
        }
    }
    if(updateRestoreButton && m_restoreButton) {
        m_restoreButton->setVisible((!updated && m_restoreButton->isVisible()) || m_tags->size() > 1);
    }
    const initializer_list<ButtonOverlay *> widgets = {m_lineEdit, m_comboBox, m_spinBoxes.first, m_spinBoxes.second};
    bool canApply = this->canApply(m_field);
    if(conversionError || !canApply) {
        QPixmap pixmap = style()->standardPixmap(QStyle::SP_MessageBoxCritical).scaled(16, 16);
        QString text;
        if(conversionError) {
            text = tr("The value of this field could not be read from the file because it couldn't be converted proberly.");
            if(!canApply) {
                text += "\n";
            }
        }
        if(!canApply) {
            text += tr("The field can not be applied when saving the file and will be lost.");
        }
        for(ButtonOverlay *overlay : widgets) {
            if(overlay) {
                overlay->enableInfoButton(pixmap, text);
            }
        }
    } else {
        for(ButtonOverlay *overlay : widgets) {
            if(overlay) {
                overlay->disableInfoButton();
            }
        }
    }
}

/*!
 * \brief Internally called by the other setup methods to create the "restore button".
 */
IconButton *TagFieldEdit::setupRestoreButton()
{
    if(!m_restoreButton) { // setup restore button
        m_restoreButton = new IconButton(this);
        m_restoreButton->setPixmap(/*QIcon::fromTheme(QStringLiteral("edit-undo"), */QIcon(QStringLiteral(":/qtutilities/icons/hicolor/16x16/actions/edit-menu.png")/*)*/).pixmap(16));
        m_restoreButton->setToolTip(tr("restore"));
        connect(m_restoreButton, &IconButton::clicked, this, &TagFieldEdit::handleRestoreButtonClicked);
        // ownership might be transfered to a child widget/layout
        connect(m_restoreButton, &IconButton::destroyed, this, &TagFieldEdit::handleRestoreButtonDestroyed);
    }
    return m_restoreButton;
}

/*!
 * \brief Internally called to show the restore button (if there is one and at least one tag is assigned).
 */
void TagFieldEdit::showRestoreButton()
{
    if(m_restoreButton) {
        m_restoreButton->setVisible(m_tags->size());
    }
}

/*!
 * \brief Applies auto correction (according to the settings) for the specified \a textValue.
 */
void TagFieldEdit::applyAutoCorrection(QString &textValue)
{
    auto &fields = Settings::autoCorrectionFields().items();
    auto i = find_if(fields.constBegin(), fields.constEnd(), [this] (const ChecklistItem &item) {
        bool ok;
        return (item.id().toInt(&ok) == static_cast<int>(this->field())) && ok;
    });
    // if current field is in the list of auto correction fields and auto correction should be applied
    if(i != fields.constEnd() && i->isChecked()) {
        if(Settings::trimWhitespaces()) {
            textValue = textValue.trimmed();
        }
        if(Settings::fixUmlauts()) {
            textValue = Utility::fixUmlauts(textValue);
        }
        if(Settings::formatNames()) {
            textValue = Utility::formatName(textValue);
        }
    }
}

/*!
 * \brief If \a previousValueHandling is PreviousValueHandling::Auto it will be set to a concrete value.
 */
void TagFieldEdit::concretizePreviousValueHandling(PreviousValueHandling &previousValueHandling)
{
    switch(previousValueHandling) {
    case PreviousValueHandling::Auto:
        switch(m_field) {
        case KnownField::Title:
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
    default:
        ;
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
    if(m_lineEdit) {
        m_lineEdit->clear();
    }
    if(m_comboBox) {
        m_comboBox->setCurrentText(QString());
    }
    if(m_plainTextEdit) {
        m_plainTextEdit->clear();
    }
    if(m_spinBoxes.first) {
        m_spinBoxes.first->setValue(0);
    }
    if(m_spinBoxes.second) {
        m_spinBoxes.second->setValue(0);
    }
    if(m_pictureSelection) {
        m_pictureSelection->clear();
    }
    if(m_descriptionLineEdit) {
        m_descriptionLineEdit->clear();
    }
}

/*!
 * \brief Applies the current value to the assigned tag.
 */
void TagFieldEdit::apply()
{
    foreach(Tag *tag, *m_tags) {
        if(m_dataType == TagDataType::Picture) {
            if(m_pictureSelection) {
                m_pictureSelection->apply();
            }
        } else {
            TagTextEncoding encoding = Settings::preferredEncoding();
            if(!tag->canEncodingBeUsed(encoding)) {
                encoding = tag->proposedTextEncoding();
            }
            tag->setValue(m_field, value(encoding, tag->supportsDescription(m_field)));
        }
    }
}

/*!
 * \brief Handles different key events.
 */
bool TagFieldEdit::eventFilter(QObject *obj, QEvent *event)
{
    switch(event->type()) {
    case QEvent::KeyRelease: {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        int key = keyEvent->key();
        switch(key) {
        case Qt::Key_Return:
            emit returnPressed();
            break;
        case Qt::Key_Shift:
            if(keyEvent->modifiers() & Qt::ControlModifier) {
                if(QLineEdit *le = qobject_cast<QLineEdit *>(obj)) {
                    le->setText(Utility::formatName(le->text()));
                    return true;
                }
            }
            break;
        default:
            ;
        }
        break;
    } default:
        ;
    }
    return QWidget::eventFilter(obj, event);
}

/*!
 * \brief Called when the restore button has been clicked.
 */
void TagFieldEdit::handleRestoreButtonClicked()
{
    if(tags().size()) {
        QMenu menu;
        int i = 0;
        foreach(Tag *tag, tags()) {
            ++i;
            QAction *action = menu.addAction(tr("restore to value from %1 (%2)").arg(tag->typeName()).arg(i));
            connect(action, &QAction::triggered, std::bind(static_cast<void (TagFieldEdit::*) (Tag *, PreviousValueHandling)>(&TagFieldEdit::updateValue), this, tag, PreviousValueHandling::Clear));
        }
        menu.exec(QCursor::pos());
    } else {
        restore();
    }
}

/*!
 * \brief Sets m_restoreButton to nullptr when the restore button has been destroyed.
 */
void TagFieldEdit::handleRestoreButtonDestroyed(QObject *obj)
{
    if(obj == m_restoreButton) {
        m_restoreButton = nullptr;
    }
}

}
