#pragma once

#include <QWidget>

#include "GameEntry.h"

class FlowLayout;
class RoundButton;

// The main window: a header (title + Pull/Bloxstrap/Token buttons), a
// scrollable quick-launch grid sized for 4 columns x 3 rows by default
// (more games just scroll), and a floating circular "+" button pinned to
// the bottom-left corner.
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

    // Remote (GitHub) sync.
    void initializeRemoteSync();
    void syncPushChanges(const QString &commitMessage);
    void handlePullClicked();
    void handleOpenBloxstrapClicked();
    void handleOpenRobloxClicked();
    void handleOpenRemoteClicked();
    void handleChangeTokenClicked();
    // Loads the saved token and re-validates it, prompting via
    // TokenPromptDialog if it's missing/invalid. Returns false if the
    // user cancels that prompt. Shared by initializeRemoteSync() and
    // handleOpenRemoteClicked().
    bool ensureValidToken(QString *tokenOut);

    FlowLayout *m_flowLayout;
    QWidget *m_cardsContainer;
    RoundButton *m_addButton;
};
