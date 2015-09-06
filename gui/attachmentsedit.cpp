#include "attachmentsedit.h"

#include "attachmentsmodel.h"

#include "gui/ui_attachmentsedit.h"

#include <tagparser/mediafileinfo.h>
#include <tagparser/abstractattachment.h>

#include <c++utilities/io/copy.h>

#include <QFileDialog>
#include <QMessageBox>

#include <fstream>

using namespace std;
using namespace IoUtilities;
using namespace Media;

namespace QtGui {

AttachmentsEdit::AttachmentsEdit(MediaFileInfo *fileInfo, QWidget *parent) :
    QWidget(parent),
    m_ui(new Ui::AttachmentsEdit),
    m_model(nullptr)
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
{}

void AttachmentsEdit::setFileInfo(Media::MediaFileInfo *fileInfo, bool updateUi)
{
    m_fileInfo = fileInfo;
    m_currentAttachments.clear();
    if(fileInfo && fileInfo->areAttachmentsSupported()) {
        if(AbstractContainer *container = fileInfo->container()) {
            auto count = container->attachmentCount();
            m_currentAttachments.reserve(count);
            for(size_t i = 0; i < count; ++i) {
                m_currentAttachments << container->attachment(i);
            }
        }
    }
    if(updateUi) {
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

bool AttachmentsEdit::addFile(const QString &path)
{
    if(fileInfo() && fileInfo()->areAttachmentsSupported() && fileInfo()->container()) {
        // create and add attachment
        auto *attachment = fileInfo()->container()->createAttachment();
        attachment->setFile(path.toLocal8Bit().data());
        attachment->setIgnored(true);
        m_addedAttachments << attachment;
        m_model->addAttachment(-1, attachment, true, path);
        return true;
    } else {
        return false;
    }
}

void AttachmentsEdit::showFileSelection()
{
    auto path = QFileDialog::getOpenFileName(this, tr("Select a file to attach"));
    if(!path.isEmpty()) {
        if(!addFile(path)) {
            QMessageBox::warning(this, QApplication::applicationName(), tr("The file couldn't be added."));
        }
    }
}

void AttachmentsEdit::extractSelected()
{
    auto selectedRows = m_ui->treeView->selectionModel()->selectedRows();
    if(selectedRows.size() == 1) {
        if(auto *attachment = m_model->attachment(selectedRows.front())) {
            if(attachment->data() && attachment->data()->size()) {
                auto fileName = QFileDialog::getSaveFileName(this, tr("Select where to store the extracted file"));
                if(!fileName.isEmpty()) {
                    auto *data = attachment->data();
                    auto &input = attachment->data()->stream();
                    fstream file;
                    file.exceptions(ios_base::badbit | ios_base::failbit);
                    try {
                        input.seekg(data->startOffset());
                        file.open(fileName.toLocal8Bit().data(), ios_base::out | ios_base::binary);
                        CopyHelper<0x1000> helper;
                        helper.copy(input, file, data->size());
                    } catch (ios_base::failure &) {
                        QMessageBox::warning(this, QApplication::applicationName(), tr("An IO error occured when extracting the attached file."));
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



}
