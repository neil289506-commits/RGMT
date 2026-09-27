#include "GameCardWidget.h"

#include <QAbstractAnimation>
#include <QColor>
#include <QEnterEvent>
#include <QFont>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QVBoxLayout>

namespace {
constexpr qreal kThumbnailRadius = 10.0;
}

QPixmap GameCardWidget::roundedPixmap(const QPixmap &source, const QSize &targetSize, qreal radius)
{
    const QPixmap scaled = source.scaled(
        targetSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);

    QPixmap result(targetSize);
    result.fill(Qt::transparent);

    QPainter painter(&result);
    painter.setRenderHint(QPainter::Antialiasing);

    QPainterPath path;
    path.addRoundedRect(QRectF(QPointF(0, 0), targetSize), radius, radius);
    painter.setClipPath(path);

    const int x = (targetSize.width() - scaled.width()) / 2;
    const int y = (targetSize.height() - scaled.height()) / 2;
    painter.drawPixmap(x, y, scaled);

    return result;
}

QPixmap GameCardWidget::placeholderThumbnail(const QSize &size, qreal radius, bool dark)
{
    QPixmap result(size);
    result.fill(Qt::transparent);

    QPainter painter(&result);
    painter.setRenderHint(QPainter::Antialiasing);

    QPainterPath path;
    path.addRoundedRect(QRectF(QPointF(0, 0), size), radius, radius);
    painter.setClipPath(path);

    QLinearGradient gradient(0, 0, 0, size.height());
    if (dark) {
        gradient.setColorAt(0, QColor("#3A3C42"));
        gradient.setColorAt(1, QColor("#2B2D31"));
    } else {
        gradient.setColorAt(0, QColor("#E7E9ED"));
        gradient.setColorAt(1, QColor("#D8DAE0"));
    }
    painter.fillRect(QRectF(QPointF(0, 0), size), gradient);

    // Generic "no image" glyph (mountains + sun), like a typical photo
    // placeholder icon — no copyrighted or branded artwork involved.
    const QColor glyphColor = dark ? QColor("#6B6E76") : QColor("#B4B8C0");
    painter.setPen(Qt::NoPen);
    painter.setBrush(glyphColor);

    const qreal w = size.width();
    const qreal h = size.height();
    painter.drawEllipse(QPointF(w * 0.32, h * 0.36), w * 0.06, w * 0.06);

    QPainterPath mountains;
    mountains.moveTo(w * 0.18, h * 0.72);
    mountains.lineTo(w * 0.40, h * 0.46);
    mountains.lineTo(w * 0.55, h * 0.62);
    mountains.lineTo(w * 0.68, h * 0.48);
    mountains.lineTo(w * 0.86, h * 0.72);
    mountains.closeSubpath();
    painter.drawPath(mountains);

    return result;
}

GameCardWidget::GameCardWidget(const GameEntry &entry, QWidget *parent)
    : QFrame(parent), m_entry(entry)
{
    setFixedSize(184, 224);
    setObjectName("gameCard");
    setAttribute(Qt::WA_Hover, true);

    const bool dark = palette().color(QPalette::Window).lightness() < 128;
    setStyleSheet(QString(
        "#gameCard { background-color: %1; border: 1px solid %2; border-radius: 12px; }")
        .arg(dark ? "#2B2D31" : "#FFFFFF", dark ? "#3A3C42" : "#E1E3E8"));

    m_shadow = new QGraphicsDropShadowEffect();
    m_shadow->setBlurRadius(18);
    m_shadow->setOffset(0, 3);
    m_shadow->setColor(QColor(0, 0, 0, dark ? 120 : 55));
    setGraphicsEffect(m_shadow);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(6);

    const QSize thumbSize(164, 100);
    m_thumbnailLabel = new QLabel(this);
    m_thumbnailLabel->setFixedSize(thumbSize);
    m_thumbnailLabel->setAlignment(Qt::AlignCenter);

    const QPixmap source(entry.thumbnailPath);
    if (!source.isNull())
        m_thumbnailLabel->setPixmap(roundedPixmap(source, thumbSize, kThumbnailRadius));
    else
        m_thumbnailLabel->setPixmap(placeholderThumbnail(thumbSize, kThumbnailRadius, dark));
    layout->addWidget(m_thumbnailLabel);

    auto *nameLabel = new QLabel(entry.name.isEmpty() ? tr("未命名遊戲") : entry.name, this);
    nameLabel->setWordWrap(true);
    QFont nameFont = nameLabel->font();
    nameFont.setBold(true);
    nameFont.setPointSize(nameFont.pointSize() + 1);
    nameLabel->setFont(nameFont);
    layout->addWidget(nameLabel);

    auto *creatorLabel = new QLabel(tr("作者：%1").arg(entry.creatorName), this);
    creatorLabel->setWordWrap(true);
    creatorLabel->setObjectName("subtitleLabel");
    layout->addWidget(creatorLabel);

    layout->addStretch();

    auto *buttonRow = new QHBoxLayout();
    buttonRow->setSpacing(6);
    auto *playButton = new QPushButton(tr("▶ 啟動"), this);
    playButton->setObjectName("primaryButton");
    auto *deleteButton = new QPushButton(tr("刪除"), this);
    buttonRow->addWidget(playButton, 1);
    buttonRow->addWidget(deleteButton);
    layout->addLayout(buttonRow);

    connect(playButton, &QPushButton::clicked, this, [this]() {
        emit playRequested(m_entry.placeId);
    });
    connect(deleteButton, &QPushButton::clicked, this, [this]() {
        emit deleteRequested(m_entry.placeId);
    });
}

void GameCardWidget::enterEvent(QEnterEvent *event)
{
    auto *animation = new QPropertyAnimation(m_shadow, "blurRadius", this);
    animation->setDuration(120);
    animation->setStartValue(m_shadow->blurRadius());
    animation->setEndValue(28);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
    m_shadow->setOffset(0, 6);
    QFrame::enterEvent(event);
}

void GameCardWidget::leaveEvent(QEvent *event)
{
    auto *animation = new QPropertyAnimation(m_shadow, "blurRadius", this);
    animation->setDuration(150);
    animation->setStartValue(m_shadow->blurRadius());
    animation->setEndValue(18);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
    m_shadow->setOffset(0, 3);
    QFrame::leaveEvent(event);
}
