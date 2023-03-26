#include "./entertargetdialog.h"

#include "ui_entertargetdialog.h"

#include <tagparser/abstracttrack.h>
#include <tagparser/matroska/matroskacontainer.h>
#include <tagparser/matroska/matroskaeditionentry.h>
#include <tagparser/matroska/matroskatagid.h>
#include <tagparser/mediafileinfo.h>
#include <tagparser/signature.h>

#include <qtutilities/misc/conversion.h>
#include <qtutilities/misc/dialogutils.h>
#include <qtutilities/models/checklistmodel.h>

#include <QMessageBox>

using namespace std;
using namespace CppUtilities;
using namespace QtUtilities;
using namespace TagParser;

namespace QtGui {

void addIds(vector<std::uint64_t> &target, ChecklistModel *sourceModel)
{
    for (const auto &item : sourceModel->items()) {
        if (item.isChecked()) {
            target.push_back(item.id().toULongLong());
        }
    }
}

template <class T> void addIds(ChecklistModel *targetModel, const T &fileIds, const vector<std::uint64_t> &targetIds)
{
    QList<std::uint64_t> ids;
    QList<ChecklistItem> items;
    for (const auto &i : fileIds) {
        items << ChecklistItem(QVariant::fromValue(i->id()), QString::fromUtf8(i->label().data()),
            std::find(targetIds.cbegin(), targetIds.cend(), i->id()) == targetIds.cend() ? Qt::Unchecked : Qt::Checked);
        ids << i->id();
    }
    for (auto id : targetIds) {
        if (!ids.contains(id)) {
            items << ChecklistItem(QVariant::fromValue(id), QStringLiteral("ID: %1").arg(id), Qt::Checked);
        }
    }
    targetModel->setItems(items);
}

void addIds(ChecklistModel *targetModel, const vector<std::uint64_t> &targetIds)
{
    QList<ChecklistItem> items;
    for (auto id : targetIds) {
        items << ChecklistItem(QVariant::fromValue(id), QStringLiteral("ID: %1").arg(id), Qt::Checked);
    }
    targetModel->setItems(items);
}

EnterTargetDialog::EnterTargetDialog(QWidget *parent)
    : QDialog(parent)
    , m_ui(new Ui::EnterTargetDialog)
    , m_tracksModel(new ChecklistModel(this))
    , m_chaptersModel(new ChecklistModel(this))
    , m_editionsModel(new ChecklistModel(this))
    , m_attachmentsModel(new ChecklistModel(this))
    , m_currentContainerFormat(ContainerFormat::Unknown)
{
    // setup UI
    m_ui->setupUi(this);
    updateStyleSheet();
    // setup views
    m_ui->tracksListView->setModel(m_tracksModel);
    m_ui->chaptersListView->setModel(m_chaptersModel);
    m_ui->editionsListView->setModel(m_editionsModel);
    m_ui->attachmentsListView->setModel(m_attachmentsModel);
    // connect signals and slots
    connect(
        m_ui->levelSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &EnterTargetDialog::updateLevelNamePlaceholderText);
    connect(m_ui->confirmPushButton, &QPushButton::clicked, this, &EnterTargetDialog::accept);
    connect(m_ui->abortPushButton, &QPushButton::clicked, this, &EnterTargetDialog::reject);
}

EnterTargetDialog::~EnterTargetDialog()
{
}

void EnterTargetDialog::updateLevelNamePlaceholderText(int i)
{
    m_ui->levelNameLineEdit->setPlaceholderText(qstringFromStdStringView(
                                                    i >= 0 ? tagTargetLevelName(containerTargetLevel(m_currentContainerFormat, static_cast<std::uint32_t>(i))) : std::string_view()));
}

void EnterTargetDialog::updateStyleSheet()
{
#ifdef Q_OS_WINDOWS
    setStyleSheet(dialogStyleForPalette(palette()));
#endif
}

TagParser::TagTarget EnterTargetDialog::target() const
{
    TagTarget target;
    target.setLevel(static_cast<std::uint64_t>(m_ui->levelSpinBox->value()));
    target.setLevelName(m_ui->levelNameLineEdit->text().toUtf8().data());
    addIds(target.tracks(), m_tracksModel);
    addIds(target.chapters(), m_chaptersModel);
    addIds(target.editions(), m_editionsModel);
    addIds(target.attachments(), m_attachmentsModel);
    return target;
}

void EnterTargetDialog::setTarget(const TagTarget &target, const MediaFileInfo *file)
{
    m_currentContainerFormat = file ? file->containerFormat() : ContainerFormat::Unknown;
    if (m_ui->levelSpinBox->maximum() >= 0 && target.level() <= static_cast<unsigned int>(m_ui->levelSpinBox->maximum())
        && (m_ui->levelSpinBox->minimum() < 0 || target.level() >= static_cast<unsigned int>(m_ui->levelSpinBox->minimum()))) {
        m_ui->levelSpinBox->setValue(static_cast<int>(target.level()));
        m_ui->levelWarningLabel->setHidden(true);
    } else {
        m_ui->levelWarningLabel->setHidden(false);
    }
    m_ui->levelNameLineEdit->setText(QString::fromUtf8(target.levelName().c_str()));
    if (file) {
        // add track IDs
        addIds(m_tracksModel, file->tracks(), target.tracks());
        // add chapter IDs
        addIds(m_chaptersModel, file->chapters(), target.chapters());
        // add edition IDs
        if (file->containerFormat() == ContainerFormat::Matroska) {
            addIds(m_editionsModel, static_cast<const MatroskaContainer *>(file->container())->editionEntires(), target.editions());
        }
        // add attachment IDs
        addIds(m_attachmentsModel, file->attachments(), target.attachments());

    } else {
        // add track IDs
        addIds(m_tracksModel, target.tracks());
        // add chapter IDs
        addIds(m_chaptersModel, target.chapters());
        // add edition IDs
        addIds(m_editionsModel, target.editions());
        // add attachment IDs
        addIds(m_attachmentsModel, target.attachments());
    }
}

bool EnterTargetDialog::event(QEvent *event)
{
    const auto res = QDialog::event(event);
    switch (event->type()) {
    case QEvent::PaletteChange:
        updateStyleSheet();
        break;
    default:;
    }
    return res;
}

} // namespace QtGui
