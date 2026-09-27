#pragma once

#include <QDialog>

class QLineEdit;
class QLabel;
class QPushButton;
class QRadioButton;
class QProgressBar;
class GitHubClient;

// Shown when no git remote is configured yet. Lets the user either
// initialize a brand-new private GitHub repository or link an existing
// one, then performs the actual git/GitHub operations before accepting.
class RemoteSetupDialog : public QDialog
{
    Q_OBJECT
public:
    RemoteSetupDialog(const QString &token, const QString &appDir, QWidget *parent = nullptr);

private:
    void handleConfirm();
    void setBusy(bool busy, const QString &status = QString());

    QRadioButton *m_initOption;
    QRadioButton *m_linkOption;
    QLineEdit *m_repoNameInput;  // used when initializing a new repo
    QLineEdit *m_repoUrlInput;   // used when linking an existing repo
    QLabel *m_statusLabel;
    QProgressBar *m_progressBar;
    QPushButton *m_confirmButton;

    QString m_token;
    QString m_appDir;
    GitHubClient *m_client;
};
