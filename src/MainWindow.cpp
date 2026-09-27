#include "MainWindow.h"

#include <QDesktopServices>
#include <QFont>
#include <QFrame>
#include <QLabel>
#include <QLayoutItem>
#include <QMessageBox>
#include <QResizeEvent>
#include <QScrollArea>
#include <QUrl>
#include <QVBoxLayout>

#include "AddGameDialog.h"
#include "FlowLayout.h"
#include "GameCardWidget.h"
#include "RoundButton.h"

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle(tr("Roblox Game Maintenance Tool"));
    resize(860, 600);

    auto *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(20, 18, 20, 18);
    outerLayout->setSpacing(4);

    auto *titleLabel = new QLabel(tr("Roblox 遊戲維護工具"), this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(titleFont.pointSize() + 6);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    outerLayout->addWidget(titleLabel);

    auto *subtitleLabel = new QLabel(tr("快速啟動"), this);
    subtitleLabel->setObjectName("subtitleLabel");
    outerLayout->addWidget(subtitleLabel);
    outerLayout->addSpacing(8);

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
}

void MainWindow::handleAddGameClicked()
{
    AddGameDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted)
        reloadGamesFromDisk();
}
