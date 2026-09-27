#pragma once

#include <QList>
#include <QString>

// One saved quick-launch entry. Persisted as Game\<PlaceId>.ini, with its
// thumbnail saved alongside as ThumbNail\<PlaceId>.png.
struct GameEntry
{
    qint64 placeId = 0;
    qint64 universeId = 0;
    QString name;
    QString creatorName;
    QString creatorType;   // "User" or "Group"
    QString thumbnailPath; // absolute path, or empty if none was saved

    // Saves this entry to Game\<placeId>.ini (creates the Game\ folder if needed).
    bool save() const;

    // Deletes Game\<placeId>.ini and ThumbNail\<placeId>.png, if they exist.
    bool remove() const;

    // Loads a single entry from Game\<placeId>.ini.
    static bool load(qint64 placeId, GameEntry *out);

    // Scans the Game\ folder and loads every saved entry.
    static QList<GameEntry> loadAll();

    // Path helpers, all relative to the application's working directory.
    static QString gameFolderPath();
    static QString thumbnailFolderPath();
    static QString iniPathFor(qint64 placeId);
    static QString thumbnailPathFor(qint64 placeId);
};
