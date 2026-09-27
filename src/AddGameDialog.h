#pragma once

#include <QDialog>

#include "GameEntry.h"
#include "RobloxApiClient.h"

class QLineEdit;
class QLabel;
class QPushButton;
class QProgressBar;

// The "+" dialog: takes a pasted link or ID, resolves it via
// RobloxApiClient, and saves the result as a GameEntry on accept.
class AddGameDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AddGameDialog(QWidget *parent = nullptr);

    // Valid only after the dialog was accepted.
    GameEntry resultEntry() const { return m_resultEntry; }

private:
    void handleAddClicked();
    void setBusy(bool busy, const QString &status = QString());

    QLineEdit *m_input;
    QLabel *m_statusLabel;
    QPushButton *m_addButton;
    QProgressBar *m_progressBar;

    RobloxApiClient *m_apiClient;
    GameEntry m_resultEntry;
};
