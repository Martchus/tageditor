#include "generalfileinfowidget.h"

#include <martchusutilities/conversion/stringconversion.h>
#include <martchusutilities/chrono/timespan.h>

#include <taginfo/mediafileinfo.h>

#include <QApplication>
#include <QPushButton>
#include <QMessageBox>
#include <QTreeView>
#include <QLabel>

using namespace std;
using namespace ConversionUtilities;
using namespace ChronoUtilities;
using namespace Media;

namespace QtGui {

GeneralFileInfoWidget::GeneralFileInfoWidget(QWidget *parent) :
    InfoWidgetBase(parent),
    m_pathLabel(nullptr),
    m_sizeLabel(nullptr),
    m_durationLabel(nullptr),
    m_bitrateLabel(nullptr),
    m_containerLabel(nullptr),
    m_mimeTypeLabel(nullptr)
{}

GeneralFileInfoWidget::~GeneralFileInfoWidget()
{}


void GeneralFileInfoWidget::update(const MediaFileInfo &fileInfo)
{
    int row = 0;

    // use file name as name
    setupName(fileInfo.fileName());

    // notifications
    setupNotifications(fileInfo);

    // path
    setupRow(row, "Path", m_pathLabel, fileInfo.path());
    ++row;

    // size
    setupRow(row, "Size", m_sizeLabel, dataSizeToString(fileInfo.size()));
    ++row;

    // overall duration and bitrate
    TimeSpan duration = fileInfo.duration();
    if(duration.isNull()) {
        removeRow(m_durationLabel);
        removeRow(m_bitrateLabel);
    } else {
        setupRow(row, "Duration", m_durationLabel, duration.toString(TimeSpanOutputFormat::WithMeasures));
        ++row;
        setupRow(row, "Overall avg. bitrate", m_bitrateLabel, bitrateToString(0.0078125 * fileInfo.size() / duration.totalSeconds()));
        ++row;
    }

    // container format
    string container(fileInfo.containerFormatName());
    const char *subversion = fileInfo.containerFormatSubversion();
    if(*subversion) {
        container.append(" ");
        container.append(subversion);
    }
    setupRow(row, "Container format", m_containerLabel, container);
    ++row;

    // mime type
    const char *mimeType = fileInfo.containerMimeType();
    if(*mimeType) {
        setupRow(row, "Mime-type", m_mimeTypeLabel, QString::fromLocal8Bit(mimeType));
        ++row;
    } else {
        removeRow(m_mimeTypeLabel);
    }
}

}
