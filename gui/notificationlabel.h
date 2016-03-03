#ifndef NOTIFICATIONLABEL_H
#define NOTIFICATIONLABEL_H

#include <QWidget>
#include <QPixmap>
#include <QTimer>

namespace QtGui {

enum class NotificationType
{
    Information,
    Warning,
    Critical,
    TaskComplete,
    Progress
};

enum class NotificationSubject
{
    None,
    Saving
};

class NotificationLabel : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText)
    Q_PROPERTY(NotificationType notificationType READ notificationType WRITE setNotificationType)
    Q_PROPERTY(NotificationSubject notificationSubject READ notificationSubject WRITE setNotificationSubject)
    Q_PROPERTY(int percentage READ percentage WRITE setPercentage)
    Q_PROPERTY(int minIconSize READ minIconSize WRITE setMinIconSize)
    Q_PROPERTY(int maxIconSize READ maxIconSize WRITE setMaxIconSize)
public:
    explicit NotificationLabel(QWidget *parent = nullptr);

    const QString &text() const;
    NotificationType notificationType() const;
    NotificationSubject notificationSubject() const;
    int percentage() const;
    int minIconSize() const;
    int maxIconSize() const;

    virtual QSize sizeHint() const;
    virtual QSize minimumSizeHint() const;

public slots:
    void setText(const QString &text);
    void clearText();
    void appendLine(const QString &line);
    void setNotificationType(NotificationType value);
    void setNotificationSubject(NotificationSubject value);
    void setPercentage(int percentage);
    void setMinIconSize(int size);
    void setMaxIconSize(int size);

protected:
    virtual void paintEvent(QPaintEvent *event);
    
private slots:
    void updateAnimation();

private:
    QRect iconRect() const;
    QRect textRect() const;
    void setupPixmaps(const QSize &size);
    void drawProgressIndicator(QPainter &painter, QRect rect, const QColor &color, int angle);
    static void toGrayPixmap(const QPixmap &original, QPixmap grayed);

    QString m_text;
    NotificationType m_type;
    NotificationSubject m_subject;
    int m_percentage;
    int m_minIconSize;
    int m_maxIconSize;
    bool m_pixmapsInvalidated;
    QPixmap m_mainPixmap;
    QPixmap m_smallPixmap;
    QPixmap m_mainPixmapDisabled;
    QPixmap m_smallPixmapDisabled;
    QTimer m_updateTimer;
    int m_animationStep;
};

inline const QString &NotificationLabel::text() const
{
    return m_text;
}

inline NotificationType NotificationLabel::notificationType() const
{
    return m_type;
}

inline NotificationSubject NotificationLabel::notificationSubject() const
{
    return m_subject;
}

inline int NotificationLabel::percentage() const
{
    return m_percentage;
}

inline int NotificationLabel::minIconSize() const
{
    return m_minIconSize;
}

inline int NotificationLabel::maxIconSize() const
{
    return m_maxIconSize;
}

}

#endif // NOTIFICATIONLABEL_H
