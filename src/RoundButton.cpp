#include "RoundButton.h"

#include <QEnterEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPen>

namespace {
constexpr const char *kAccent = "#E2241D";
constexpr const char *kAccentHover = "#F13B33";
constexpr const char *kAccentPressed = "#C21B15";
}

RoundButton::RoundButton(QWidget *parent)
    : QPushButton(parent)
{
    setFixedSize(48, 48);
    setCursor(Qt::PointingHandCursor);
    setFlat(true);
    // Suppress the platform's default button chrome; everything visible
    // is drawn in paintEvent() instead.
    setStyleSheet("QPushButton { border: none; background: transparent; }");
}

void RoundButton::enterEvent(QEnterEvent *event)
{
    m_hovered = true;
    update();
    QPushButton::enterEvent(event);
}

void RoundButton::leaveEvent(QEvent *event)
{
    m_hovered = false;
    update();
    QPushButton::leaveEvent(event);
}

void RoundButton::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const QRectF bounds(1, 1, width() - 2.0, height() - 2.0);
    const QColor fill(isDown() ? kAccentPressed : (m_hovered ? kAccentHover : kAccent));

    // Soft shadow beneath the circle.
    QPainterPath shadowPath;
    shadowPath.addEllipse(bounds.translated(0, 2));
    painter.fillPath(shadowPath, QColor(0, 0, 0, 55));

    QPainterPath circlePath;
    circlePath.addEllipse(bounds);
    painter.fillPath(circlePath, fill);

    // "+" glyph.
    painter.setPen(QPen(Qt::white, 2.4, Qt::SolidLine, Qt::RoundCap));
    const qreal armLength = bounds.width() * 0.26;
    const QPointF c = bounds.center();
    painter.drawLine(QPointF(c.x() - armLength, c.y()), QPointF(c.x() + armLength, c.y()));
    painter.drawLine(QPointF(c.x(), c.y() - armLength), QPointF(c.x(), c.y() + armLength));
}
