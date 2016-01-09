#include "./infowidgetbase.h"
#include "./notificationmodel.h"

#include <c++utilities/conversion/stringconversion.h>
#include <c++utilities/chrono/timespan.h>

#include <tagparser/mediafileinfo.h>

#include <QApplication>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QTreeView>
#include <QLabel>
#include <QLineEdit>

using namespace std;
using namespace ConversionUtilities;
using namespace ChronoUtilities;
using namespace Media;

namespace QtGui {

InfoWidgetBase::InfoWidgetBase(QWidget *parent) :
    QWidget(parent),
    m_notificationModel(nullptr)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    m_formLayout = new QFormLayout(this);
    mainLayout->addLayout(m_formLayout);

    QHBoxLayout *m_bottomLayout = new QHBoxLayout(this);
    m_bottomLayout->setAlignment(Qt::AlignBottom | Qt::AlignRight);

    m_notificationsButton = new QPushButton(tr("Show notifications"), this);
    m_notificationsButton->setHidden(true);
    connect(m_notificationsButton, &QPushButton::clicked, this, &InfoWidgetBase::showNotifications);
    m_bottomLayout->addWidget(m_notificationsButton);

    mainLayout->addLayout(m_bottomLayout);

    setLayout(mainLayout);
}

InfoWidgetBase::~InfoWidgetBase()
{}

void InfoWidgetBase::setupName(const string &name)
{
    m_name = QString::fromLocal8Bit(name.c_str());
}

void InfoWidgetBase::setupNotifications(const StatusProvider &provider)
{
    if(provider.hasNotifications()) {
        m_notificationsButton->setHidden(false);
        if(provider.hasCriticalNotifications()) {
            m_notificationsButton->setIcon(NotificationModel::errorIcon());
        } else {
            m_notificationsButton->setIcon(NotificationModel::warningIcon());
        }
        if(!m_notificationModel) {
            m_notificationModel = new NotificationModel(this);
        }
        m_notificationModel->setNotifications(provider.notifications());
    } else {
        m_notificationsButton->setHidden(true);
    }
}

void InfoWidgetBase::showNotifications()
{
    if(m_notificationModel) {
        QDialog dlg;
        dlg.setWindowFlags(Qt::Tool);
        dlg.setWindowTitle(tr("Notifications for %1 - %2").arg(m_name).arg(QApplication::applicationName()));
        QPoint point = m_notificationsButton->mapToGlobal(QPoint(m_notificationsButton->size().width(), m_notificationsButton->size().height()));
        dlg.resize(600, 350);
        dlg.move(point.x() - 600, point.y());
        QHBoxLayout layout;
        layout.setMargin(0);
        QTreeView view;
        view.setIndentation(0);
        view.setFrameStyle(QFrame::NoFrame);
        view.setModel(m_notificationModel);
        view.setWordWrap(true);
        view.setColumnWidth(0, 100);
        view.setColumnWidth(1, 350);
        layout.addWidget(&view);
        dlg.setLayout(&layout);
        dlg.exec();
    } else {
        QMessageBox::warning(this, windowTitle(), tr("There are no notifications to be shown."));
    }
}

void InfoWidgetBase::setupRow(int index, const char *labelText, QLabel *&label, const string &text)
{
    setupRow(index, labelText, label, QString::fromLocal8Bit(text.c_str()));
}

void InfoWidgetBase::setupRow(int index, const char *labelText, QLabel *&label, const QString &text)
{
    if(!label) {
        label = new QLabel(this);
        label->setTextInteractionFlags(Qt::TextBrowserInteraction);
        label->setWordWrap(true);
        m_formLayout->insertRow(index, tr(labelText), label);
    } else {
        int currentIndex = m_formLayout->indexOf(label);
        if(currentIndex != (index * 2 + 1)) {
            if(QWidget *labelWidget = m_formLayout->labelForField(label)) {
                labelWidget->deleteLater();
            }
            m_formLayout->removeWidget(label);
            m_formLayout->insertRow(index, tr(labelText), label);
        }
    }
    label->setText(text);
}

void InfoWidgetBase::setupRow(int index, const char *labelText, QLineEdit *&edit, const string &text)
{
    setupRow(index, labelText, edit, QString::fromLocal8Bit(text.c_str()));
}

void InfoWidgetBase::setupRow(int index, const char *labelText, QLineEdit *&edit, const QString &text)
{
    if(!edit) {
        edit = new QLineEdit(this);
        m_formLayout->insertRow(index, tr(labelText), edit);
    } else {
        int currentIndex = m_formLayout->indexOf(edit);
        if(currentIndex != (index * 2 + 1)) {
            if(QWidget *labelWidget = m_formLayout->labelForField(edit)) {
                labelWidget->deleteLater();
            }
            m_formLayout->removeWidget(edit);
            m_formLayout->insertRow(index, tr(labelText), edit);
        }
    }
    edit->setText(text);
    edit->setPlaceholderText(tr("empty"));
}

void InfoWidgetBase::removeRow(QLabel *&label)
{
    if(QWidget *labelWidget = m_formLayout->labelForField(label)) {
        labelWidget->deleteLater();
    }
    label->deleteLater();
    label = nullptr;
}

void InfoWidgetBase::removeRow(QLineEdit *&edit)
{
    if(QWidget *labelWidget = m_formLayout->labelForField(edit)) {
        labelWidget->deleteLater();
    }
    edit->deleteLater();
    edit = nullptr;
}

}
