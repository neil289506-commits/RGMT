#include "RobloxApiClient.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QUrl>

RobloxApiClient::RobloxApiClient(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{
}

qint64 RobloxApiClient::extractIdFromInput(const QString &input)
{
    const QString trimmed = input.trimmed();

    // Plain numeric ID, e.g. "606849621"
    bool ok = false;
    const qint64 direct = trimmed.toLongLong(&ok);
    if (ok && direct > 0)
        return direct;

    // Roblox link, e.g. https://www.roblox.com/games/606849621/Some-Game-Name
    static const QRegularExpression re(R"(roblox\.com/games/(\d+))");
    const QRegularExpressionMatch match = re.match(trimmed);
    if (match.hasMatch())
        return match.captured(1).toLongLong();

    // Fallback: grab the first run of 3+ digits anywhere in the string.
    static const QRegularExpression anyDigits(R"((\d{3,}))");
    const QRegularExpressionMatch fallback = anyDigits.match(trimmed);
    if (fallback.hasMatch())
        return fallback.captured(1).toLongLong();

    return 0;
}

void RobloxApiClient::resolveUniverse(
    qint64 candidateId,
    std::function<void(qint64, QString)> onDone)
{
    // Assume the pasted ID is a PlaceId first (the common case for links
    // copied from the Roblox website), and ask Roblox to convert it.
    const QUrl url(QString("https://apis.roblox.com/universes/v1/places/%1/universe").arg(candidateId));
    QNetworkReply *reply = m_nam->get(QNetworkRequest(url));

    connect(reply, &QNetworkReply::finished, this, [reply, candidateId, onDone]() {
        reply->deleteLater();

        if (reply->error() == QNetworkReply::NoError) {
            const QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
            const qint64 universeId = obj.value("universeId").toVariant().toLongLong();
            if (universeId > 0) {
                onDone(universeId, QString());
                return;
            }
        }

        // The conversion failed, which means candidateId was most likely
        // already a UniverseId (not a PlaceId) — use it as-is and let the
        // games API validate it next.
        onDone(candidateId, QString());
    });
}

void RobloxApiClient::fetchGameInfo(
    qint64 universeId,
    std::function<void(bool, GameInfo, QString)> onDone)
{
    const QUrl url(QString("https://games.roblox.com/v1/games?universeIds=%1").arg(universeId));
    QNetworkReply *reply = m_nam->get(QNetworkRequest(url));

    connect(reply, &QNetworkReply::finished, this, [reply, universeId, onDone]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            onDone(false, GameInfo{}, tr("查詢遊戲資訊失敗：%1").arg(reply->errorString()));
            return;
        }

        const QJsonArray data = QJsonDocument::fromJson(reply->readAll())
                                     .object().value("data").toArray();
        if (data.isEmpty()) {
            onDone(false, GameInfo{}, tr("找不到對應的遊戲，請確認連結或 ID 是否正確。"));
            return;
        }

        const QJsonObject game = data.first().toObject();
        GameInfo info;
        info.universeId = universeId;
        info.placeId = game.value("rootPlaceId").toVariant().toLongLong();
        info.name = game.value("name").toString();

        const QJsonObject creator = game.value("creator").toObject();
        info.creatorName = creator.value("name").toString();
        info.creatorType = creator.value("type").toString();

        onDone(true, info, QString());
    });
}

void RobloxApiClient::fetchThumbnail(
    qint64 universeId,
    std::function<void(bool, QByteArray, QString)> onDone)
{
    const QUrl iconUrl(QString(
        "https://thumbnails.roblox.com/v1/games/icons"
        "?universeIds=%1&size=512x512&format=Png&isCircular=false").arg(universeId));
    QNetworkReply *iconReply = m_nam->get(QNetworkRequest(iconUrl));

    connect(iconReply, &QNetworkReply::finished, this, [this, iconReply, onDone]() {
        iconReply->deleteLater();

        if (iconReply->error() != QNetworkReply::NoError) {
            onDone(false, QByteArray(), tr("查詢縮圖失敗：%1").arg(iconReply->errorString()));
            return;
        }

        const QJsonArray data = QJsonDocument::fromJson(iconReply->readAll())
                                     .object().value("data").toArray();
        if (data.isEmpty()) {
            onDone(false, QByteArray(), tr("找不到縮圖。"));
            return;
        }

        const QString imageUrl = data.first().toObject().value("imageUrl").toString();
        if (imageUrl.isEmpty()) {
            onDone(false, QByteArray(), tr("縮圖尚未產生，請稍後再試。"));
            return;
        }

        QNetworkReply *imgReply = m_nam->get(QNetworkRequest(QUrl(imageUrl)));
        connect(imgReply, &QNetworkReply::finished, this, [imgReply, onDone]() {
            imgReply->deleteLater();
            if (imgReply->error() != QNetworkReply::NoError) {
                onDone(false, QByteArray(), tr("下載縮圖失敗：%1").arg(imgReply->errorString()));
                return;
            }
            onDone(true, imgReply->readAll(), QString());
        });
    });
}

void RobloxApiClient::fetchGameByLinkOrId(
    const QString &linkOrId,
    std::function<void(bool, GameInfo, QByteArray, QString)> onDone)
{
    const qint64 candidateId = extractIdFromInput(linkOrId);
    if (candidateId <= 0) {
        onDone(false, GameInfo{}, QByteArray(),
               tr("無法從輸入內容解析出遊戲 ID，請貼上完整連結或數字 ID。"));
        return;
    }

    resolveUniverse(candidateId, [this, onDone](qint64 universeId, QString err) {
        if (universeId <= 0) {
            onDone(false, GameInfo{}, QByteArray(),
                   err.isEmpty() ? tr("無法解析 Universe ID。") : err);
            return;
        }

        fetchGameInfo(universeId, [this, onDone](bool ok, GameInfo info, QString err) {
            if (!ok) {
                onDone(false, GameInfo{}, QByteArray(), err);
                return;
            }

            fetchThumbnail(info.universeId, [info, onDone](bool ok2, QByteArray png, QString err2) {
                if (!ok2)
                    onDone(true, info, QByteArray(), err2); // thumbnail failure is non-fatal
                else
                    onDone(true, info, png, QString());
            });
        });
    });
}
