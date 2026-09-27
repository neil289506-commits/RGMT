#include "RemoteSetupDialog.h"

#include "GitHubClient.h"
#include "GitRunner.h"

#include <QLabel>
#include <QLineEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QRegularExpression>
#include <QVBoxLayout>

namespace {

// Normalizes a full GitHub URL or an "owner/repo" shorthand into an
// HTTPS clone URL with the token embedded for non-interactive auth,
// e.g. https://<token>@github.com/owner/repo.git
QString buildAuthUrl(const QString &ownerRepoOrUrl, const QString &token)
{
    QString ownerRepo = ownerRepoOrUrl.trimmed();

    static const QRegularExpression re(R"(github\.com[:/]+([^/]+/[^/.]+))");
    const auto match = re.match(ownerRepo);
    if (match.hasMatch())
        ownerRepo = match.captured(1);

    if (ownerRepo.endsWith(".git"))
        ownerRepo.chop(4);

    return QStringLiteral("https://%1@github.com/%2.git").arg(token, ownerRepo);
}

} // namespace

RemoteSetupDialog::RemoteSetupDialog(const QString &token, const QString &appDir, QWidget *parent)
    : QDialog(parent)
    , m_token(token)
    , m_appDir(appDir)
    , m_client(new GitHubClient(this))
{
    setWindowTitle(tr("Set Up Remote Sync"));
    setMinimumWidth(440);

    auto *layout = new QVBoxLayout(this);
    auto *introLabel = new QLabel(
        tr("No remote repository is linked yet. Choose how you'd like to sync your quick-launch list:"),
        this);
    introLabel->setWordWrap(true);
    layout->addWidget(introLabel);

    m_initOption = new QRadioButton(tr("Create a new private GitHub repository"), this);
    m_initOption->setChecked(true);
    layout->addWidget(m_initOption);

    m_repoNameInput = new QLineEdit(this);
    m_repoNameInput->setPlaceholderText(tr("Repository name, e.g. rgmt-data"));
    m_repoNameInput->setText("rgmt-data");
    layout->addWidget(m_repoNameInput);

    m_linkOption = new QRadioButton(tr("Link an existing repository"), this);
    layout->addWidget(m_linkOption);

    m_repoUrlInput = new QLineEdit(this);
    m_repoUrlInput->setPlaceholderText(tr("owner/repo or full GitHub URL"));
    m_repoUrlInput->setEnabled(false);
    layout->addWidget(m_repoUrlInput);

    connect(m_initOption, &QRadioButton::toggled, this, [this](bool checked) {
        m_repoNameInput->setEnabled(checked);
        m_repoUrlInput->setEnabled(!checked);
    });

    m_statusLabel = new QLabel(this);
    m_statusLabel->setWordWrap(true);
    layout->addWidget(m_statusLabel);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 0);
    m_progressBar->setVisible(false);
    layout->addWidget(m_progressBar);

    m_confirmButton = new QPushButton(tr("Continue"), this);
    m_confirmButton->setObjectName("primaryButton");
    layout->addWidget(m_confirmButton);

    connect(m_confirmButton, &QPushButton::clicked, this, &RemoteSetupDialog::handleConfirm);
}

void RemoteSetupDialog::setBusy(bool busy, const QString &status)
{
    m_confirmButton->setEnabled(!busy);
    m_initOption->setEnabled(!busy);
    m_linkOption->setEnabled(!busy);
    m_repoNameInput->setEnabled(!busy && m_initOption->isChecked());
    m_repoUrlInput->setEnabled(!busy && m_linkOption->isChecked());
    m_progressBar->setVisible(busy);
    m_statusLabel->setText(status);
}

void RemoteSetupDialog::handleConfirm()
{
    if (!GitRunner::isGitAvailable()) {
        m_statusLabel->setText(tr("git was not found on PATH. Please install Git for Windows first."));
        return;
    }

    if (m_initOption->isChecked()) {
        const QString repoName = m_repoNameInput->text().trimmed();
        if (repoName.isEmpty()) {
            m_statusLabel->setText(tr("Please enter a repository name."));
            return;
        }

        setBusy(true, tr("Creating private repository on GitHub..."));
        m_client->createPrivateRepo(m_token, repoName,
            [this](bool ok, QString cloneUrl, QString error) {
                if (!ok) {
                    setBusy(false, tr("Failed: %1").arg(error));
                    return;
                }

                setBusy(true, tr("Initializing local repository..."));
                const QString authUrl = buildAuthUrl(cloneUrl, m_token);
                QString err;

                if (!GitRunner::isRepo(m_appDir) && !GitRunner::init(m_appDir, &err)) {
                    setBusy(false, tr("git init failed: %1").arg(err));
                    return;
                }
                if (!GitRunner::setRemote(m_appDir, authUrl, &err)) {
                    setBusy(false, tr("Could not set remote: %1").arg(err));
                    return;
                }
                GitRunner::ensureGitIgnore(m_appDir);
                if (!GitRunner::commitAll(m_appDir, "Initial commit", &err)) {
                    setBusy(false, tr("git commit failed: %1").arg(err));
                    return;
                }
                if (!GitRunner::forcePushMaster(m_appDir, &err)) {
                    setBusy(false, tr("git push failed: %1").arg(err));
                    return;
                }

                accept();
            });
        return;
    }

    // Link an existing repository.
    const QString ownerRepo = m_repoUrlInput->text().trimmed();
    if (ownerRepo.isEmpty()) {
        m_statusLabel->setText(tr("Please enter a repository (owner/repo or full URL)."));
        return;
    }

    setBusy(true, tr("Linking repository and pulling existing data..."));
    const QString authUrl = buildAuthUrl(ownerRepo, m_token);
    QString err;

    if (!GitRunner::isRepo(m_appDir) && !GitRunner::init(m_appDir, &err)) {
        setBusy(false, tr("git init failed: %1").arg(err));
        return;
    }
    if (!GitRunner::setRemote(m_appDir, authUrl, &err)) {
        setBusy(false, tr("Could not set remote: %1").arg(err));
        return;
    }
    GitRunner::ensureGitIgnore(m_appDir);
    if (!GitRunner::pull(m_appDir, &err)) {
        setBusy(false, tr("git pull failed: %1").arg(err));
        return;
    }

    accept();
}
