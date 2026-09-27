#include "GitHubClient.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

GitHubClient::GitHubClient(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{
}

void GitHubClient::validateToken(
    const QString &token,
    std::function<void(bool, QString, QString)> onDone)
{
    QNetworkRequest request{QUrl("https://api.github.com/user")};
    request.setRawHeader("Authorization", QByteArray("token ") + token.toUtf8());
    request.setRawHeader("User-Agent", "RGMT");
    request.setRawHeader("Accept", "application/vnd.github+json");

    QNetworkReply *reply = m_nam->get(request);
    connect(reply, &QNetworkReply::finished, this, [reply, onDone]() {
        reply->deleteLater();

        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status == 200) {
            const QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
            onDone(true, obj.value("login").toString(), QString());
            return;
        }
        if (status == 401) {
            onDone(false, QString(), tr("GitHub token is invalid or has expired."));
            return;
        }
        onDone(false, QString(), tr("GitHub token check failed (HTTP %1).").arg(status));
    });
}

void GitHubClient::createPrivateRepo(
    const QString &token, const QString &repoName,
    std::function<void(bool, QString, QString)> onDone)
{
    QNetworkRequest request{QUrl("https://api.github.com/user/repos")};
    request.setRawHeader("Authorization", QByteArray("token ") + token.toUtf8());
    request.setRawHeader("User-Agent", "RGMT");
    request.setRawHeader("Accept", "application/vnd.github+json");
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject body;
    body["name"] = repoName;
    body["private"] = true;
    body["auto_init"] = false; // we push the initial local content ourselves

    QNetworkReply *reply = m_nam->post(request, QJsonDocument(body).toJson());
    connect(reply, &QNetworkReply::finished, this, [reply, onDone]() {
        reply->deleteLater();

        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();

        if (status == 201) {
            onDone(true, obj.value("clone_url").toString(), QString());
            return;
        }

        const QString message = obj.value("message").toString();
        onDone(false, QString(),
               message.isEmpty() ? tr("Repository creation failed (HTTP %1).").arg(status) : message);
    });
}
