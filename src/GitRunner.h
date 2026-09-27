#pragma once

#include <QString>

// Thin synchronous wrapper around the `git` command-line tool (must be on
// PATH). Each call pumps the Qt event loop while git runs — the same
// pattern used by BloxstrapManager::installViaWinget() — so the UI never
// appears frozen during a push/pull/clone.
class GitRunner
{
public:
    static bool isGitAvailable();
    static bool isRepo(const QString &dir);
    static bool currentRemoteUrl(const QString &dir, QString *urlOut);

    static bool init(const QString &dir, QString *error = nullptr);
    static bool setRemote(const QString &dir, const QString &url, QString *error = nullptr);
    static bool commitAll(const QString &dir, const QString &message, QString *error = nullptr);
    static bool forcePushMaster(const QString &dir, QString *error = nullptr);
    static bool pull(const QString &dir, QString *error = nullptr);
    static bool clone(const QString &url, const QString &destDir, QString *error = nullptr);

    // Writes a .gitignore (if one doesn't already exist) so only Game/ and
    // ThumbNail/ are tracked — the app's own binaries never get committed.
    static void ensureGitIgnore(const QString &dir);

private:
    static bool run(const QString &workingDir, const QStringList &args,
                     QString *stdOutOut, QString *error);
};
