#pragma once

#include <QWidget>

#include "GameEntry.h"

class FlowLayout;
class RoundButton;

// The main window: a header, a scrollable quick-launch grid, and a
// floating circular "+" button pinned to the bottom-left corner.
class MainWindow : public QWidget
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void reloadGamesFromDisk();
    void addCardForEntry(const GameEntry &entry);
    void handlePlay(qint64 placeId);
    void handleDelete(qint64 placeId);
    void handleAddGameClicked();
    void updateAddButtonPosition();

    FlowLayout *m_flowLayout;
    QWidget *m_cardsContainer;
    RoundButton *m_addButton;
};
