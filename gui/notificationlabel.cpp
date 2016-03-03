#include "./notificationlabel.h"

#include <QPainter>
#include <QPaintEvent>
#include <QStyleOption>
#include <QPixmap>
#include <QFontMetrics>
#include <QBrush>
#include <QConicalGradient>
#include <QDebug>

#include <cmath>

namespace QtGui {

NotificationLabel::NotificationLabel(QWidget *parent) :
    QWidget(parent),
    m_type(NotificationType::Information),
    m_subject(NotificationSubject::None),
    m_percentage(-1),
    m_minIconSize(25),
    m_maxIconSize(25),
    m_pixmapsInvalidated(true),
    m_animationStep(0)
{
    connect(&m_updateTimer, &QTimer::timeout, this, &NotificationLabel::updateAnimation);
    m_updateTimer.setInterval(80);
}

void NotificationLabel::paintEvent(QPaintEvent *event)
{
    QStyle *style = QWidget::style();
    QStyleOption option;
    option.init(this);
    int iconSize = option.rect.height() > m_maxIconSize ? m_maxIconSize : option.rect.height();
    QRect pixmapRect(option.rect.x(), option.rect.y(), iconSize, iconSize);
    pixmapRect.moveCenter(QRect(option.rect.x(), option.rect.y(), iconSize, option.rect.height()).center());
    QRect textRect(option.rect.x() + iconSize + 5, option.rect.y(), option.rect.width() - iconSize - 5, option.rect.height());
    QPainter painter(this);

    if(event->rect().contains(textRect)) {
        style->drawItemText(&painter, textRect, QStyle::visualAlignment(layoutDirection(), Qt::AlignVCenter | Qt::AlignLeft | Qt::AlignJustify), option.palette, isEnabled(), m_text, foregroundRole());
    }
    if(event->rect().contains(pixmapRect)) {
        setupPixmaps(pixmapRect.size());
        switch(m_type) {
        case NotificationType::Progress:
            if(m_subject == NotificationSubject::None) {
                drawProgressIndicator(painter, pixmapRect, option.palette.color(QPalette::Foreground), m_animationStep);
            } else {
                style->drawItemPixmap(&painter, pixmapRect, Qt::AlignTop | Qt::AlignLeft, m_mainPixmap);
                QSize size = pixmapRect.size() * 0.3;
                pixmapRect.adjust(size.width(), size.height(), 0, 0);
                drawProgressIndicator(painter, pixmapRect, option.palette.color(QPalette::Foreground), m_animationStep);
            }
            break;
        default:
            if(m_subject == NotificationSubject::None) {
                style->drawItemPixmap(&painter, pixmapRect, Qt::AlignCenter, m_mainPixmap);
            } else {
                style->drawItemPixmap(&painter, pixmapRect, Qt::AlignTop | Qt::AlignLeft, m_mainPixmap);
                style->drawItemPixmap(&painter, pixmapRect, Qt::AlignBottom | Qt::AlignRight, m_smallPixmap);
            }
        }
    }
}

void NotificationLabel::updateAnimation()
{
    switch(m_type) {
    case NotificationType::Progress:
        m_animationStep = m_animationStep >= 15 ? m_animationStep - 15 : (360 - 15);
        break;
    default:
        ;
    }
    update(iconRect());
}

QRect NotificationLabel::iconRect() const
{
    QRect rect = this->rect();
    int iconSize = rect.height() > m_maxIconSize ? m_maxIconSize : rect.height();
    QRect pixmapRect(rect.x(), rect.y(), iconSize, iconSize);
    pixmapRect.moveCenter(QRect(rect.x(), rect.y(), iconSize, rect.height()).center());
    return pixmapRect;
}

QRect NotificationLabel::textRect() const
{
    QRect rect = this->rect();
    int iconSize = rect.height() > m_maxIconSize ? m_maxIconSize : rect.height();
    return QRect(rect.x() + iconSize + 5, rect.y(), rect.width() - iconSize - 5, rect.height());
}

void NotificationLabel::setupPixmaps(const QSize &size)
{
    QStyle *style = QWidget::style();
    if(m_pixmapsInvalidated) {
        m_mainPixmap = QPixmap();
        m_smallPixmap = QPixmap();
        QStyle::StandardPixmap icon = static_cast<QStyle::StandardPixmap>(QStyle::SP_CustomBase + 1);
        switch(m_type) {
        case NotificationType::Information:
            icon = QStyle::SP_MessageBoxInformation;
            break;
        case NotificationType::Warning:
            icon = QStyle::SP_MessageBoxWarning;
            break;
        case NotificationType::Critical:
            icon = QStyle::SP_MessageBoxCritical;
            break;
        case NotificationType::TaskComplete:
            icon = QStyle::SP_DialogApplyButton;
            break;
        default:
            ;
        }
        switch(m_subject) {
        case NotificationSubject::None:
            if(icon != QStyle::SP_CustomBase + 1) {
                m_mainPixmap = style->standardIcon(icon, nullptr, this).pixmap(size);
            }
            break;
        case NotificationSubject::Saving:
            m_mainPixmap = style->standardIcon(QStyle::SP_DialogSaveButton, nullptr, this).pixmap(size * 0.8);
            if(icon != QStyle::SP_CustomBase + 1) {
                m_smallPixmap = style->standardIcon(icon, nullptr, this).pixmap(size * 0.6);
            }
            break;
        }
        m_pixmapsInvalidated = false;
    }
}

void NotificationLabel::drawProgressIndicator(QPainter &painter, QRect rect, const QColor &color, int angle)
{
    qreal circleWidth = 6;
    qreal halfCircleWidth = circleWidth * 0.5;
    rect.adjust(halfCircleWidth + 1, halfCircleWidth + 1, -halfCircleWidth - 1, -halfCircleWidth - 1);
    QPointF center = rect.center();
    painter.setRenderHint(QPainter::Antialiasing, true);
    QConicalGradient linearGrad(center, angle);
    linearGrad.setColorAt(0, color);
    linearGrad.setColorAt(1, Qt::transparent);
    painter.setPen(QPen(linearGrad, circleWidth, Qt::SolidLine, Qt::RoundCap));
    painter.drawArc(rect, angle * 16 + 25 * 16, 16 * 310);
    if(m_percentage > 0 && m_percentage <= 100) {
        QFont font = this->font();
        font.setPixelSize(rect.width() * 0.5);
        painter.setFont(font);
        painter.setPen(color);
        painter.drawText(rect, Qt::AlignCenter, QString::number(m_percentage));
    }
}

QSize NotificationLabel::sizeHint() const
{
    return minimumSizeHint();
}

QSize NotificationLabel::minimumSizeHint() const
{
    QFontMetrics fm(font());
    QSize size = fm.size(0, m_text);
    if(size.height() < m_minIconSize) {
        size.setHeight(m_minIconSize);
    }
    int iconSize = size.height() > m_maxIconSize ? m_maxIconSize : size.height();
    size.setWidth(iconSize + 5 + size.width());
    return size;
}

void NotificationLabel::setText(const QString &text)
{
    m_text = text;
    updateGeometry();
    update(textRect());
}

void NotificationLabel::clearText()
{
    m_text.clear();
    updateGeometry();
    update(textRect());
}

void NotificationLabel::appendLine(const QString &line)
{
    if(m_text.isEmpty()) {
        m_text = line;
    } else {
        if(!m_text.startsWith("•")) {
            m_text.insert(0, "• ");
        }
        m_text.append("\n• ");
        m_text.append(line);
    }
    updateGeometry();
    update(textRect());
}

void NotificationLabel::setNotificationSubject(NotificationSubject value)
{
    m_subject = value;
    m_pixmapsInvalidated = true;
    update(iconRect());
}

void NotificationLabel::setNotificationType(NotificationType value)
{
    if(m_type != value) {
        m_type = value;
        m_pixmapsInvalidated = true;
        m_animationStep = 0;
        update(iconRect());
        if(m_type == NotificationType::Progress) {
            m_updateTimer.start();
        } else {
            m_updateTimer.stop();
        }
    }
}

void NotificationLabel::setPercentage(int percentage)
{
    if(m_percentage != percentage) {
        m_percentage = percentage;
        update(iconRect());
    }
}

void NotificationLabel::setMaxIconSize(int size)
{
    m_maxIconSize = size;
    if(m_minIconSize > size) {
        m_minIconSize = size;
    }
    m_pixmapsInvalidated = true;
    updateGeometry();
}

void NotificationLabel::setMinIconSize(int size)
{
    m_minIconSize = size;
    if(m_maxIconSize < size) {
        m_maxIconSize = size;
    }
    m_pixmapsInvalidated = true;
    updateGeometry();
}

}
