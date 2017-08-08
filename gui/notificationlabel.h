#ifndef NOTIFICATIONLABEL_H
#define NOTIFICATIONLABEL_H

#include <QWidget>
#include <QPixmap>
#include <QTimer>

#include <limits>

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
    Q_PROPERTY(QString context READ context WRITE setContext)
    Q_PROPERTY(NotificationType notificationType READ notificationType WRITE setNotificationType)
    Q_PROPERTY(NotificationSubject notificationSubject READ notificationSubject WRITE setNotificationSubject)
    Q_PROPERTY(int percentage READ percentage WRITE setPercentage)
    Q_PROPERTY(int minIconSize READ minIconSize WRITE setMinIconSize)
    Q_PROPERTY(int maxIconSize READ maxIconSize WRITE setMaxIconSize)
    Q_PROPERTY(std::size_t maxLineCount READ maxLineCount WRITE setMaxLineCount)
    Q_PROPERTY(std::size_t currentLineCount READ currentLineCount)

public:
    explicit NotificationLabel(QWidget *parent = nullptr);

    const QString &text() const;
    const QString &context() const;
    NotificationType notificationType() const;
    NotificationSubject notificationSubject() const;
    int percentage() const;
    int minIconSize() const;
    int maxIconSize() const;
    std::size_t maxLineCount() const;
    std::size_t currentLineCount() const;

    virtual QSize sizeHint() const;
    virtual QSize minimumSizeHint() const;

public slots:
    void setText(const QString &text);
    void setContext(const QString &context);
    void clearText();
    void appendLine(const QString &line);
    void setNotificationType(NotificationType value);
    void setNotificationSubject(NotificationSubject value);
    void setPercentage(int percentage);
    void setMinIconSize(int size);
    void setMaxIconSize(int size);
    void setMaxLineCount(std::size_t maxLineCount);

protected:
    void paintEvent(QPaintEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    
private slots:
    void updateAnimation();
    void showMessageBox() const;

private:
    QRect iconRect() const;
    QRect textRect() const;
    void setupPixmaps(const QSize &size);
    void drawProgressIndicator(QPainter &painter, QRect rect, const QColor &color, int angle);
    void applyMaxLineCount();

public:
    static constexpr std::size_t infiniteLines = std::numeric_limits<std::size_t>::max();

private:
    QString m_text;
    QString m_context;
    NotificationType m_type;
    NotificationSubject m_subject;
    int m_percentage;
    int m_minIconSize;
    int m_maxIconSize;
    bool m_pixmapsInvalidated;
    QPixmap m_mainPixmap;
    QPixmap m_smallPixmap;
    QTimer m_updateTimer;
    int m_animationStep;
    std::size_t m_maxLineCount;
    std::size_t m_currentLineCount;
    static const QChar s_bulletPoint;
    static const QString s_bulletLine;
};

inline const QString &NotificationLabel::text() const
{
    return m_text;
}

inline const QString &NotificationLabel::context() const
{
    return m_context;
}

inline void NotificationLabel::setContext(const QString &context)
{
    m_context = context;
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

inline std::size_t NotificationLabel::maxLineCount() const
{
    return m_maxLineCount;
}

inline std::size_t NotificationLabel::currentLineCount() const
{
    return m_currentLineCount;
}

}

#endif // NOTIFICATIONLABEL_H
