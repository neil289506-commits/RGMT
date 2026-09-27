#pragma once

#include <QObject>
#include <QString>

#include <functional>

// Thin wrapper around the public Roblox HTTP APIs needed to resolve a
// game link/ID into full game info + thumbnail. All endpoints below are
// public and require no authentication.
//
//  - Place -> Universe:  GET https://apis.roblox.com/universes/v1/places/{placeId}/universe
//  - Game info:          GET https://games.roblox.com/v1/games?universeIds={universeId}
//  - Icon lookup:        GET https://thumbnails.roblox.com/v1/games/icons?universeIds={universeId}&size=512x512&format=Png
class RobloxApiClient : public QObject
{
    Q_OBJECT
public:
    struct GameInfo
    {
        qint64 placeId = 0;
        qint64 universeId = 0;
        QString name;
        QString creatorName;
        QString creatorType;
    };

    explicit RobloxApiClient(QObject *parent = nullptr);

    // Extracts a numeric Place/Universe ID out of a pasted Roblox link
    // (e.g. https://www.roblox.com/games/606849621/...) or a bare numeric
    // ID string. Returns 0 if no ID could be found.
    static qint64 extractIdFromInput(const QString &input);

    // Full pipeline: resolve ID -> universe -> game info -> thumbnail bytes.
    // Calls onDone(success, info, thumbnailPngBytes, errorMessage) exactly
    // once. A thumbnail fetch failure alone is treated as non-fatal (the
    // game is still considered successfully resolved, just with empty
    // thumbnail bytes).
    void fetchGameByLinkOrId(
        const QString &linkOrId,
        std::function<void(bool, GameInfo, QByteArray, QString)> onDone);

private:
    // Tries the ID as a PlaceId first (the common case for pasted links).
    // If Roblox rejects that, the ID is assumed to already be a UniverseId.
    void resolveUniverse(
        qint64 candidateId,
        std::function<void(qint64 universeId, QString error)> onDone);

    void fetchGameInfo(
        qint64 universeId,
        std::function<void(bool ok, GameInfo info, QString error)> onDone);

    void fetchThumbnail(
        qint64 universeId,
        std::function<void(bool ok, QByteArray png, QString error)> onDone);

    class QNetworkAccessManager *m_nam;
};
