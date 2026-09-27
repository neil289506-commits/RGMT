#include "MainWindow.h"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QEventLoop>
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayoutItem>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QUrl>
#include <QVBoxLayout>

#include "AddGameDialog.h"
#include "BloxstrapManager.h"
#include "FlowLayout.h"
#include "GameCardWidget.h"
#include "GitHubClient.h"
#include "GitRunner.h"
#include "RemoteSetupDialog.h"
#include "RoundButton.h"
#include "SecureTokenStore.h"
#include "TokenPromptDialog.h"

namespace {
// Card is 184x224, FlowLayout spacing is 14, its own margin is 4.
// 4 columns: 4*184 + 3*14 + 2*4 = 786. 3 rows: 3*224 + 2*14 + 2*4 = 708.
// The window size below leaves that grid fully visible without scrolling;
// a 5th column or 4th row just scrolls (QScrollArea's default wheel
// handling — no extra code needed).
constexpr int kDefaultWindowWidth = 860;
constexpr int kDefaultWindowHeight = 900;
}

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle(tr("Roblox Game Maintenance Tool"));
    resize(kDefaultWindowWidth, kDefaultWindowHeight);

    auto *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(20, 18, 20, 18);
    outerLayout->setSpacing(4);

    // --- Header: title/subtitle on the left, remote-sync actions on the right.
    auto *headerRow = new QHBoxLayout();

    auto *titleColumn = new QVBoxLayout();
    auto *titleLabel = new QLabel(tr("Roblox 遊戲維護工具"), this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(titleFont.pointSize() + 6);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleColumn->addWidget(titleLabel);

    auto *subtitleLabel = new QLabel(tr("快速啟動"), this);
    subtitleLabel->setObjectName("subtitleLabel");
    titleColumn->addWidget(subtitleLabel);
    headerRow->addLayout(titleColumn);
    headerRow->addStretch();

    auto *pullButton = new QPushButton(tr("⭳ Pull"), this);
    auto *robloxButton = new QPushButton(tr("▶ 快速啟動 Roblox"), this);
    auto *bloxstrapButton = new QPushButton(tr("▶ Bloxstrap"), this);
    auto *remoteButton = new QPushButton(tr("🔗 Remote"), this);
    auto *tokenButton = new QPushButton(tr("🔑 Token"), this);
    headerRow->addWidget(pullButton);
    headerRow->addWidget(robloxButton);
    headerRow->addWidget(bloxstrapButton);
    headerRow->addWidget(remoteButton);
    headerRow->addWidget(tokenButton);

    outerLayout->addLayout(headerRow);
    outerLayout->addSpacing(8);

    connect(pullButton, &QPushButton::clicked, this, &MainWindow::handlePullClicked);
    connect(robloxButton, &QPushButton::clicked, this, &MainWindow::handleOpenRobloxClicked);
    connect(bloxstrapButton, &QPushButton::clicked, this, &MainWindow::handleOpenBloxstrapClicked);
    connect(remoteButton, &QPushButton::clicked, this, &MainWindow::handleOpenRemoteClicked);
    connect(tokenButton, &QPushButton::clicked, this, &MainWindow::handleChangeTokenClicked);

    // --- Quick-launch grid.
    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    m_cardsContainer = new QWidget(scrollArea);
    m_flowLayout = new FlowLayout(m_cardsContainer, 4, 14, 14);
    m_cardsContainer->setLayout(m_flowLayout);

    scrollArea->setWidget(m_cardsContainer);
    outerLayout->addWidget(scrollArea, 1);

    // Floating "+" button, pinned to the bottom-left corner as requested —
    // positioned by updateAddButtonPosition() rather than laid out inline,
    // so it stays put regardless of scrolling or window resizing.
    m_addButton = new RoundButton(this);
    m_addButton->setToolTip(tr("新增遊戲"));
    connect(m_addButton, &QPushButton::clicked, this, &MainWindow::handleAddGameClicked);

    // Remote sync setup/token check/pull happens once, before the first
    // load, so a fresh pull is reflected immediately.
    initializeRemoteSync();

    reloadGamesFromDisk();
    updateAddButtonPosition();
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateAddButtonPosition();
}

void MainWindow::updateAddButtonPosition()
{
    constexpr int margin = 24;
    m_addButton->move(margin, height() - m_addButton->height() - margin);
    m_addButton->raise();
}

void MainWindow::reloadGamesFromDisk()
{
    QLayoutItem *item;
    while ((item = m_flowLayout->takeAt(0))) {
        delete item->widget();
        delete item;
    }

    const QList<GameEntry> entries = GameEntry::loadAll();
    for (const GameEntry &entry : entries)
        addCardForEntry(entry);
}

void MainWindow::addCardForEntry(const GameEntry &entry)
{
    auto *card = new GameCardWidget(entry, m_cardsContainer);
    connect(card, &GameCardWidget::playRequested, this, &MainWindow::handlePlay);
    connect(card, &GameCardWidget::deleteRequested, this, &MainWindow::handleDelete);
    m_flowLayout->addWidget(card);
}

void MainWindow::handlePlay(qint64 placeId)
{
    // Verified against the Bloxstrap wiki ("A deep dive on how the Roblox
    // bootstrapper works"): roblox://experiences/start?placeId=<id> is the
    // exact scheme Roblox's own website uses to launch the client, and
    // Bloxstrap (once registered as the roblox-player handler) intercepts
    // roblox-player launches the same way the stock bootstrapper does.
    const QUrl url(QString("roblox://experiences/start?placeId=%1").arg(placeId));
    if (!QDesktopServices::openUrl(url)) {
        QMessageBox::warning(this, tr("啟動失敗"),
            tr("無法透過 roblox:// 通訊協定啟動遊戲，請確認 Bloxstrap 已正確安裝並至少手動執行過一次。"));
    }
}

void MainWindow::handleDelete(qint64 placeId)
{
    GameEntry entry;
    QString displayName = QString::number(placeId);
    if (GameEntry::load(placeId, &entry) && !entry.name.isEmpty())
        displayName = entry.name;
    entry.placeId = placeId;

    const auto choice = QMessageBox::question(this, tr("確認刪除"),
        tr("確定要從快速啟動移除「%1」嗎？此動作無法復原。").arg(displayName),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (choice != QMessageBox::Yes)
        return;

    entry.remove();
    reloadGamesFromDisk();
    syncPushChanges(tr("Remove game %1").arg(placeId));
}

void MainWindow::handleAddGameClicked()
{
    AddGameDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        reloadGamesFromDisk();
        syncPushChanges(tr("Add game %1").arg(dialog.resultEntry().placeId));
    }
}

// ---------------------------------------------------------------------
// Remote (GitHub) sync
// ---------------------------------------------------------------------

bool MainWindow::ensureValidToken(QString *tokenOut)
{
    QString token;
    bool haveValidToken = false;

    if (SecureTokenStore::loadToken(&token, nullptr)) {
        GitHubClient client;
        QEventLoop loop;
        bool valid = false;
        client.validateToken(token, [&](bool ok, QString, QString) {
            valid = ok;
            loop.quit();
        });
        loop.exec();
        haveValidToken = valid;
    }

    if (!haveValidToken) {
        TokenPromptDialog dlg(this,
            SecureTokenStore::hasToken()
                ? tr("Your saved GitHub token is invalid or has expired. Please enter a new one:")
                : tr("Enter a GitHub Personal Access Token (needs 'repo' scope) to enable remote sync:"));
        if (dlg.exec() != QDialog::Accepted)
            return false; // user cancelled
        token = dlg.token();
    }

    if (tokenOut)
        *tokenOut = token;
    return true;
}

void MainWindow::initializeRemoteSync()
{
    const QString appDir = QCoreApplication::applicationDirPath();

    // 1) Make sure we have a valid GitHub token, prompting if it's
    //    missing, invalid, or expired (supports switching to a new one).
    QString token;
    if (!ensureValidToken(&token))
        return; // user chose to skip remote sync for this session

    // 2) Make sure a remote repository is configured.
    QString remoteUrl;
    const bool hasRemote = GitRunner::isRepo(appDir) && GitRunner::currentRemoteUrl(appDir, &remoteUrl);

    if (!hasRemote) {
        RemoteSetupDialog setupDialog(token, appDir, this);
        if (setupDialog.exec() != QDialog::Accepted)
            return; // user chose not to set up remote sync yet
    }

    // 3) Pull the latest data every time the app opens.
    QString pullError;
    if (!GitRunner::pull(appDir, &pullError)) {
        QMessageBox::warning(this, tr("Pull Failed"),
            tr("Could not pull the latest data from the remote repository:\n%1").arg(pullError));
    }
}

void MainWindow::syncPushChanges(const QString &commitMessage)
{
    const QString appDir = QCoreApplication::applicationDirPath();
    QString remoteUrl;
    if (!GitRunner::isRepo(appDir) || !GitRunner::currentRemoteUrl(appDir, &remoteUrl))
        return; // remote sync isn't configured — nothing to push

    GitRunner::ensureGitIgnore(appDir);

    QString error;
    if (!GitRunner::commitAll(appDir, commitMessage, &error)
        || !GitRunner::forcePushMaster(appDir, &error)) {
        QMessageBox::warning(this, tr("Sync Failed"),
            tr("Could not push your change to the remote repository:\n%1").arg(error));
    }
}

void MainWindow::handlePullClicked()
{
    const QString appDir = QCoreApplication::applicationDirPath();
    QString error;
    if (!GitRunner::pull(appDir, &error)) {
        QMessageBox::warning(this, tr("Pull Failed"), error);
        return;
    }
    reloadGamesFromDisk();
}

void MainWindow::handleOpenRobloxClicked()
{
    // Bare roblox:// with no parameters — just hands off to the
    // roblox-player/roblox protocol handler (Bloxstrap) with nothing to
    // launch, no placeId, no experience.
    QDesktopServices::openUrl(QUrl("roblox://"));
}

void MainWindow::handleOpenBloxstrapClicked()
{
    // Launching Bloxstrap.exe directly (no roblox:// arguments) opens its
    // own menu/settings window, rather than joining an experience — this
    // is different from the roblox-player:1+launchmode:app URI, which
    // launches through the protocol handler instead.
    const QString exePath = BloxstrapManager::installedExecutablePath();
    if (exePath.isEmpty()) {
        QMessageBox::warning(this, tr("找不到 Bloxstrap"),
            tr("找不到 Bloxstrap.exe，請確認已安裝。"));
        return;
    }

    if (!QProcess::startDetached(exePath, {})) {
        QMessageBox::warning(this, tr("啟動失敗"), tr("無法啟動 Bloxstrap。"));
    }
}

void MainWindow::handleOpenRemoteClicked()
{
    QString token;
    if (!ensureValidToken(&token))
        return;

    const QString appDir = QCoreApplication::applicationDirPath();
    RemoteSetupDialog setupDialog(token, appDir, this);
    if (setupDialog.exec() == QDialog::Accepted)
        reloadGamesFromDisk();
}

void MainWindow::handleChangeTokenClicked()
{
    TokenPromptDialog dlg(this, tr("Enter a new GitHub Personal Access Token:"));
    dlg.exec(); // saving happens inside the dialog on success
}
