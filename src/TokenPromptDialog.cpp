#include "TokenPromptDialog.h"

#include "SecureTokenStore.h"

#include <QLabel>
#include <QLineEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QVBoxLayout>

TokenPromptDialog::TokenPromptDialog(QWidget *parent, const QString &reason)
    : QDialog(parent)
    , m_client(new GitHubClient(this))
{
    setWindowTitle(tr("GitHub Token"));
    setMinimumWidth(400);

    auto *layout = new QVBoxLayout(this);

    const QString intro = reason.isEmpty()
        ? tr("Enter a GitHub Personal Access Token (needs 'repo' scope):")
        : reason;
    auto *introLabel = new QLabel(intro, this);
    introLabel->setWordWrap(true);
    layout->addWidget(introLabel);

    m_tokenInput = new QLineEdit(this);
    m_tokenInput->setEchoMode(QLineEdit::Password);
    m_tokenInput->setPlaceholderText(tr("ghp_..."));
    layout->addWidget(m_tokenInput);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setWordWrap(true);
    layout->addWidget(m_statusLabel);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 0);
    m_progressBar->setVisible(false);
    layout->addWidget(m_progressBar);

    m_saveButton = new QPushButton(tr("Validate && Save"), this);
    m_saveButton->setObjectName("primaryButton");
    layout->addWidget(m_saveButton);

    connect(m_saveButton, &QPushButton::clicked, this, &TokenPromptDialog::handleSaveClicked);
    connect(m_tokenInput, &QLineEdit::returnPressed, this, &TokenPromptDialog::handleSaveClicked);
}

void TokenPromptDialog::setBusy(bool busy, const QString &status)
{
    m_tokenInput->setEnabled(!busy);
    m_saveButton->setEnabled(!busy);
    m_progressBar->setVisible(busy);
    m_statusLabel->setText(status);
}

void TokenPromptDialog::handleSaveClicked()
{
    const QString token = m_tokenInput->text().trimmed();
    if (token.isEmpty()) {
        m_statusLabel->setText(tr("Please enter a token."));
        return;
    }

    setBusy(true, tr("Validating token with GitHub..."));

    m_client->validateToken(token, [this, token](bool valid, QString login, QString error) {
        Q_UNUSED(login);
        if (!valid) {
            setBusy(false, tr("Failed: %1").arg(error));
            return;
        }

        QString saveError;
        if (!SecureTokenStore::saveToken(token, &saveError)) {
            setBusy(false, tr("Token is valid, but saving it locally failed: %1").arg(saveError));
            return;
        }

        m_validatedToken = token;
        accept();
    });
}
