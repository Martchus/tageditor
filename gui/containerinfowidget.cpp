#include "containerinfowidget.h"

#include <taginfo/mediafileinfo.h>
#include <taginfo/abstractcontainer.h>

#include <QLabel>
#include <QLineEdit>

using namespace std;
using namespace Media;

namespace QtGui {

ContainerInfoWidget::ContainerInfoWidget(QWidget *parent) :
    InfoWidgetBase(parent),
    m_nameLabel(nullptr),
    m_abbreviationLabel(nullptr),
    m_versionLabel(nullptr),
    m_readVersionLabel(nullptr),
    m_docTypeLabel(nullptr),
    m_docVersionLabel(nullptr),
    m_docReadVersionLabel(nullptr),
    m_titleLabel(nullptr)
{}

ContainerInfoWidget::~ContainerInfoWidget()
{}

void ContainerInfoWidget::update(const Media::MediaFileInfo &fileInfo, const Media::AbstractContainer &container)
{
    int row = 0;

    // use container format name as name
    string name(fileInfo.containerFormatName());
    const char *subversion = fileInfo.containerFormatSubversion();
    if(*subversion) {
        name.append(" ");
        name.append(subversion);
    }
    setupName(name);
    setupRow(row, "Name", m_nameLabel, name);
    ++row;

    // notifications
    setupNotifications(container);

    // abbreviation
    const char *abbreviation = fileInfo.containerFormatAbbreviation();
    if(*abbreviation) {
        setupRow(row, "Abbreviation", m_abbreviationLabel, QString::fromLocal8Bit(abbreviation));
        ++row;
    } else {
        removeRow(m_abbreviationLabel);
    }

    // version
    if(container.version()) {
        setupRow(row, "Version", m_versionLabel, QString::number(container.version()));
        ++row;
    } else {
        removeRow(m_versionLabel);
    }
    if(container.readVersion()) {
        setupRow(row, "Read version", m_readVersionLabel, QString::number(container.readVersion()));
        ++row;
    } else {
        removeRow(m_docReadVersionLabel);
    }

    // document
    if(!container.documentType().empty()) {
        setupRow(row, "Document type", m_docTypeLabel, container.documentType());
        ++row;
    } else {
        removeRow(m_docTypeLabel);
    }
    if(container.doctypeVersion()) {
        setupRow(row, "Document version", m_docVersionLabel, QString::number(container.doctypeVersion()));
        ++row;
    } else {
        removeRow(m_docVersionLabel);
    }
    if(container.doctypeReadVersion()) {
        setupRow(row, "Document read version", m_docReadVersionLabel, QString::number(container.doctypeReadVersion()));
        ++row;
    } else {
        removeRow(m_docReadVersionLabel);
    }
    setupRow(row, "Title", m_titleLabel, container.title());
    m_titleLabel->setReadOnly(fileInfo.containerFormat() != ContainerFormat::Matroska);

}

}
