#include "AddGameDialog.h"

#include <QDir>
#include <QFile>
#include <QLabel>
#include <QLineEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QVBoxLayout>

AddGameDialog::AddGameDialog(QWidget *parent)
    : QDialog(parent)
    , m_apiClient(new RobloxApiClient(this))
{
    setWindowTitle(tr("新增遊戲"));
    setMinimumWidth(380);

    auto *layout = new QVBoxLayout(this);

    layout->addWidget(new QLabel(tr("請貼上遊戲連結，或直接輸入 PlaceID / UniverseID："), this));

    m_input = new QLineEdit(this);
    m_input->setPlaceholderText(tr("例如：https://www.roblox.com/games/606849621/... 或 606849621"));
    layout->addWidget(m_input);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setWordWrap(true);
    layout->addWidget(m_statusLabel);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 0); // indeterminate
    m_progressBar->setVisible(false);
    layout->addWidget(m_progressBar);

    m_addButton = new QPushButton(tr("新增"), this);
    m_addButton->setObjectName("primaryButton");
    layout->addWidget(m_addButton);

    connect(m_addButton, &QPushButton::clicked, this, &AddGameDialog::handleAddClicked);
    connect(m_input, &QLineEdit::returnPressed, this, &AddGameDialog::handleAddClicked);
}

void AddGameDialog::setBusy(bool busy, const QString &status)
{
    m_input->setEnabled(!busy);
    m_addButton->setEnabled(!busy);
    m_progressBar->setVisible(busy);
    m_statusLabel->setText(status);
}

void AddGameDialog::handleAddClicked()
{
    const QString text = m_input->text().trimmed();
    if (text.isEmpty()) {
        m_statusLabel->setText(tr("請輸入遊戲連結或 ID。"));
        return;
    }

    setBusy(true, tr("正在查詢遊戲資訊..."));

    m_apiClient->fetchGameByLinkOrId(text,
        [this](bool ok, RobloxApiClient::GameInfo info, QByteArray thumbnailPng, QString error) {
            if (!ok) {
                setBusy(false, tr("失敗：%1").arg(error));
                return;
            }

            GameEntry entry;
            entry.placeId = info.placeId;
            entry.universeId = info.universeId;
            entry.name = info.name;
            entry.creatorName = info.creatorName;
            entry.creatorType = info.creatorType;
            entry.thumbnailPath = GameEntry::thumbnailPathFor(entry.placeId);

            if (!thumbnailPng.isEmpty()) {
                QDir().mkpath(GameEntry::thumbnailFolderPath());
                QFile thumbFile(entry.thumbnailPath);
                if (thumbFile.open(QIODevice::WriteOnly)) {
                    thumbFile.write(thumbnailPng);
                    thumbFile.close();
                } else {
                    entry.thumbnailPath.clear();
                }
            } else {
                entry.thumbnailPath.clear();
            }

            if (!entry.save()) {
                setBusy(false, tr("儲存遊戲資料失敗，請確認資料夾寫入權限。"));
                return;
            }

            m_resultEntry = entry;
            accept();
        });
}
