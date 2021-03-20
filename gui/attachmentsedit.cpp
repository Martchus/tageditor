#include "./attachmentsedit.h"

#include "./attachmentsmodel.h"

#include "ui_attachmentsedit.h"

#include <tagparser/abstractattachment.h>
#include <tagparser/diagnostics.h>
#include <tagparser/mediafileinfo.h>
#include <tagparser/progressfeedback.h>

#include <qtutilities/misc/conversion.h>

#include <c++utilities/io/copy.h>
#include <c++utilities/io/nativefilestream.h>

#include <QFileDialog>
#include <QMessageBox>

#include <fstream>

using namespace std;
using namespace CppUtilities;
using namespace QtUtilities;
using namespace TagParser;

namespace QtGui {

AttachmentsEdit::AttachmentsEdit(MediaFileInfo *fileInfo, QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::AttachmentsEdit)
    , m_model(nullptr)
{
    // setup UI and model
    m_ui->setupUi(this);
    m_model = new AttachmentsModel(this);
    m_ui->treeView->setModel(m_model);
    // connect signals and slots
    connect(m_ui->restorePushButton, &QPushButton::clicked, this, &AttachmentsEdit::restore);
    connect(m_ui->clearPushButton, &QPushButton::clicked, this, &AttachmentsEdit::clear);
    connect(m_ui->addPushButton, &QPushButton::clicked, this, &AttachmentsEdit::showFileSelection);
    connect(m_ui->extractPushButton, &QPushButton::clicked, this, &AttachmentsEdit::extractSelected);
    connect(m_ui->treeView->selectionModel(), &QItemSelectionModel::currentRowChanged, this, &AttachmentsEdit::selectionChanged);
    // load specified file
    setFileInfo(fileInfo, true);
}

AttachmentsEdit::~AttachmentsEdit()
{
}

void AttachmentsEdit::setFileInfo(TagParser::MediaFileInfo *fileInfo, bool updateUi)
{
    m_fileInfo = fileInfo;
    m_currentAttachments.clear();
    if (fileInfo && fileInfo->areAttachmentsSupported()) {
        if (AbstractContainer *container = fileInfo->container()) {
            auto count = container->attachmentCount();
            m_currentAttachments.reserve(static_cast<decltype(m_currentAttachments)::size_type>(count));
            for (size_t i = 0; i < count; ++i) {
                m_currentAttachments << container->attachment(i);
            }
        }
    }
    if (updateUi) {
        setEnabled(fileInfo && fileInfo->areAttachmentsSupported());
        setupUi();
    }
}

void AttachmentsEdit::clear()
{
    m_model->repealSelection();
}

void AttachmentsEdit::restore()
{
    m_model->revert();
}

void AttachmentsEdit::apply()
{
    m_model->submit();
}

void AttachmentsEdit::invalidate()
{
    // clear UI status
    // TODO
    // re-setup UI
    setupUi();
}

void AttachmentsEdit::addFile(const QString &path)
{
    if (!fileInfo() || fileInfo()->attachmentsParsingStatus() != ParsingStatus::Ok || !fileInfo()->container()) {
        throw Failure();
    }
    // create and add attachment
    auto *const attachment = fileInfo()->container()->createAttachment();
    attachment->setIgnored(true);
    Diagnostics diag;
    AbortableProgressFeedback progress; // FIXME: actually use the progress object
    attachment->setFile(toNativeFileName(path).data(), diag, progress);
    // TODO: show diag messages
    m_addedAttachments << attachment;
    m_model->addAttachment(-1, attachment, true, path);
}

void AttachmentsEdit::showFileSelection()
{
    auto path = QFileDialog::getOpenFileName(this, tr("Select a file to attach"));
    if (!path.isEmpty()) {
        try {
            addFile(path);
        } catch (const Failure &) {
            QMessageBox::warning(this, QApplication::applicationName(),
                tr("The file couldn't be added because the attachments of the file could not be parsed successfully."));
        } catch (const std::ios_base::failure &failure) {
            QMessageBox::warning(this, QApplication::applicationName(),
                tr("The file couldn't be added because an IO error occured: ") + QString::fromLocal8Bit(failure.what()));
        }
    }
}

void AttachmentsEdit::extractSelected()
{
    auto selectedRows = m_ui->treeView->selectionModel()->selectedRows();
    if (selectedRows.size() == 1) {
        if (auto *attachment = m_model->attachment(selectedRows.front())) {
            if (attachment->data() && attachment->data()->size()) {
                auto fileName = QFileDialog::getSaveFileName(this, tr("Select where to store the extracted file"));
                if (!fileName.isEmpty()) {
                    auto *data = attachment->data();
                    auto &input = attachment->data()->stream();
                    NativeFileStream file;
                    file.exceptions(ios_base::badbit | ios_base::failbit);
                    try {
                        input.seekg(static_cast<std::streamoff>(data->startOffset()), std::ios_base::beg);
                        file.open(toNativeFileName(fileName).data(), ios_base::out | ios_base::binary);
                        CopyHelper<0x1000> helper;
                        helper.copy(input, file, data->size());
                    } catch (const std::ios_base::failure &failure) {
                        QMessageBox::warning(this, QApplication::applicationName(),
                            tr("An IO error occured when extracting the attached file: ") + QString::fromLocal8Bit(failure.what()));
                    }
                }
            } else {
            }
            return;
        }
    }
    QMessageBox::warning(this, QApplication::applicationName(), tr("There must be exactly one attachment selected."));
}

void AttachmentsEdit::selectionChanged(const QModelIndex &current, const QModelIndex &)
{
    m_ui->extractPushButton->setEnabled(current.isValid());
}

void AttachmentsEdit::setupUi()
{
    static const QString currentlyAttached(tr("currently attached"));
    m_model->setAttachments(m_currentAttachments, true, currentlyAttached);
}

} // namespace QtGui
