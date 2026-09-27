#include "GameEntry.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>

QString GameEntry::gameFolderPath()
{
    return QDir(QCoreApplication::applicationDirPath()).filePath("Game");
}

QString GameEntry::thumbnailFolderPath()
{
    return QDir(QCoreApplication::applicationDirPath()).filePath("ThumbNail");
}

QString GameEntry::iniPathFor(qint64 placeId)
{
    return QDir(gameFolderPath()).filePath(QString::number(placeId) + ".ini");
}

QString GameEntry::thumbnailPathFor(qint64 placeId)
{
    return QDir(thumbnailFolderPath()).filePath(QString::number(placeId) + ".png");
}

bool GameEntry::save() const
{
    QDir().mkpath(gameFolderPath());

    QSettings ini(iniPathFor(placeId), QSettings::IniFormat);
    ini.beginGroup("Game");
    ini.setValue("PlaceId", placeId);
    ini.setValue("UniverseId", universeId);
    ini.setValue("Name", name);
    ini.setValue("CreatorName", creatorName);
    ini.setValue("CreatorType", creatorType);
    ini.setValue("ThumbnailPath", thumbnailPath);
    ini.endGroup();
    ini.sync();

    return ini.status() == QSettings::NoError;
}

bool GameEntry::remove() const
{
    bool ok = true;

    QFile ini(iniPathFor(placeId));
    if (ini.exists())
        ok = ini.remove() && ok;

    QFile thumb(thumbnailPathFor(placeId));
    if (thumb.exists())
        ok = thumb.remove() && ok;

    return ok;
}

bool GameEntry::load(qint64 placeId, GameEntry *out)
{
    const QString path = iniPathFor(placeId);
    if (!QFile::exists(path))
        return false;

    QSettings ini(path, QSettings::IniFormat);
    ini.beginGroup("Game");
    out->placeId = ini.value("PlaceId").toLongLong();
    out->universeId = ini.value("UniverseId").toLongLong();
    out->name = ini.value("Name").toString();
    out->creatorName = ini.value("CreatorName").toString();
    out->creatorType = ini.value("CreatorType").toString();
    out->thumbnailPath = ini.value("ThumbnailPath").toString();
    ini.endGroup();

    return out->placeId != 0;
}

QList<GameEntry> GameEntry::loadAll()
{
    QList<GameEntry> result;

    QDir dir(gameFolderPath());
    if (!dir.exists())
        return result;

    const QStringList iniFiles = dir.entryList({"*.ini"}, QDir::Files);
    for (const QString &fileName : iniFiles) {
        const QString baseName = QFileInfo(fileName).completeBaseName();
        bool ok = false;
        const qint64 placeId = baseName.toLongLong(&ok);
        if (!ok)
            continue;

        GameEntry entry;
        if (load(placeId, &entry))
            result.append(entry);
    }

    return result;
}
