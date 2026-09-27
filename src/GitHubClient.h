#pragma once

#include <QObject>
#include <QString>

#include <functional>

// Thin wrapper around the GitHub REST API, used only for:
//  - validating a Personal Access Token (GET /user)
//  - creating a new private repository under that account (POST /user/repos)
class GitHubClient : public QObject
{
    Q_OBJECT
public:
    explicit GitHubClient(QObject *parent = nullptr);

    // 200 -> valid (also returns the login name); 401 -> invalid/expired.
    void validateToken(const QString &token,
                        std::function<void(bool valid, QString login, QString error)> onDone);

    // Creates a private repo named `repoName` under the token's account.
    // On success, cloneUrl is the plain HTTPS clone URL (no token in it).
    void createPrivateRepo(const QString &token, const QString &repoName,
                            std::function<void(bool ok, QString cloneUrl, QString error)> onDone);

private:
    class QNetworkAccessManager *m_nam;
};
