#pragma once

#include <QDialog>

#include "GitHubClient.h"

class QLineEdit;
class QLabel;
class QPushButton;
class QProgressBar;

// Prompts for a GitHub Personal Access Token, validates it against the
// GitHub API, and saves it via SecureTokenStore on success. Reused both
// for first-time setup and for re-entering an expired/changed token.
class TokenPromptDialog : public QDialog
{
    Q_OBJECT
public:
    explicit TokenPromptDialog(QWidget *parent = nullptr, const QString &reason = QString());

    // Valid only after the dialog was accepted.
    QString token() const { return m_validatedToken; }

private:
    void handleSaveClicked();
    void setBusy(bool busy, const QString &status = QString());

    QLineEdit *m_tokenInput;
    QLabel *m_statusLabel;
    QPushButton *m_saveButton;
    QProgressBar *m_progressBar;
    GitHubClient *m_client;
    QString m_validatedToken;
};
