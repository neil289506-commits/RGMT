#pragma once

#include <QFrame>

#include "GameEntry.h"

class QLabel;
class QGraphicsDropShadowEffect;

// A single quick-launch tile: rounded thumbnail, name, creator, Play/Delete
// buttons, with a soft drop shadow that lifts slightly on hover.
class GameCardWidget : public QFrame
{
    Q_OBJECT
public:
    explicit GameCardWidget(const GameEntry &entry, QWidget *parent = nullptr);

    qint64 placeId() const { return m_entry.placeId; }

signals:
    void playRequested(qint64 placeId);
    void deleteRequested(qint64 placeId);

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    // Scales+crops `source` to fill `targetSize`, then clips it to a
    // rounded rect so thumbnails match the card's rounded corners.
    static QPixmap roundedPixmap(const QPixmap &source, const QSize &targetSize, qreal radius);

    // Drawn "no thumbnail" placeholder (soft gradient + generic photo icon)
    // used when a game has no saved thumbnail.
    static QPixmap placeholderThumbnail(const QSize &size, qreal radius, bool dark);

    GameEntry m_entry;
    QLabel *m_thumbnailLabel;
    QGraphicsDropShadowEffect *m_shadow;
};
