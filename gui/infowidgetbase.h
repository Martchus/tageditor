#ifndef INFOWIDGETBASE_H
#define INFOWIDGETBASE_H

#include <QWidget>

#include <string>

QT_BEGIN_NAMESPACE
class QFormLayout;
class QHBoxLayout;
class QLabel;
class QLineEdit;
class QPushButton;
QT_END_NAMESPACE

namespace Media {
class MediaFileInfo;
class StatusProvider;
}

namespace QtGui {

class NotificationModel;

class InfoWidgetBase : public QWidget
{
    Q_OBJECT
public:
    explicit InfoWidgetBase(QWidget *parent = nullptr);
    ~InfoWidgetBase();

protected:
    void setupName(const std::string &name);
    void setupNotifications(const Media::StatusProvider &provider);
    void setupRow(int index, const char *labelText, QLabel *&label, const std::string &text);
    void setupRow(int index, const char *labelText, QLabel *&label, const QString &text);
    void setupRow(int index, const char *labelText, QLineEdit *&edit, const std::string &text);
    void setupRow(int index, const char *labelText, QLineEdit *&edit, const QString &text);
    void removeRow(QLabel *&label);
    void removeRow(QLineEdit *&edit);

private slots:
    void showNotifications();

private:
    QFormLayout *m_formLayout;
    QHBoxLayout *m_bottomLayout;
    QPushButton *m_notificationsButton;
    NotificationModel *m_notificationModel;
    QString m_name;
};

}

#endif // INFOWIDGETBASE_H
