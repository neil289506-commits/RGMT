#include "GitRunner.h"

#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QObject>
#include <QProcess>

bool GitRunner::run(const QString &workingDir, const QStringList &args,
                     QString *stdOutOut, QString *error)
{
    QProcess process;
    if (!workingDir.isEmpty())
        process.setWorkingDirectory(workingDir);
    process.setProgram("git");
    process.setArguments(args);
    process.start();

    if (!process.waitForStarted(5000)) {
        if (error) *error = QObject::tr("Could not start git. Is Git for Windows installed and on PATH?");
        return false;
    }

    QEventLoop loop;
    QObject::connect(&process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                      &loop, &QEventLoop::quit);
    loop.exec();

    const QByteArray out = process.readAllStandardOutput();
    const QByteArray err = process.readAllStandardError();
    if (stdOutOut)
        *stdOutOut = QString::fromUtf8(out);

    const bool ok = (process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0);
    if (!ok && error)
        *error = QString::fromUtf8(err.isEmpty() ? out : err);
    return ok;
}

bool GitRunner::isGitAvailable()
{
    QString out, err;
    return run(QString(), {"--version"}, &out, &err);
}

bool GitRunner::isRepo(const QString &dir)
{
    QString out, err;
    return run(dir, {"rev-parse", "--is-inside-work-tree"}, &out, &err);
}

bool GitRunner::currentRemoteUrl(const QString &dir, QString *urlOut)
{
    QString out, err;
    if (!run(dir, {"remote", "get-url", "origin"}, &out, &err))
        return false;
    if (urlOut)
        *urlOut = out.trimmed();
    return true;
}

bool GitRunner::init(const QString &dir, QString *error)
{
    QString out;
    if (!run(dir, {"init"}, &out, error))
        return false;
    // Force the default branch name to "master", since that's what this
    // app always force-pushes to.
    run(dir, {"checkout", "-B", "master"}, &out, error);
    return true;
}

bool GitRunner::setRemote(const QString &dir, const QString &url, QString *error)
{
    QString out, ignored;
    // The remote may already exist (re-linking) — try updating it first.
    if (run(dir, {"remote", "set-url", "origin", url}, &out, &ignored))
        return true;
    return run(dir, {"remote", "add", "origin", url}, &out, error);
}

bool GitRunner::commitAll(const QString &dir, const QString &message, QString *error)
{
    QString out, ignored;
    run(dir, {"add", "-A"}, &out, &ignored);

    QString commitError;
    if (run(dir, {"commit", "-m", message}, &out, &commitError))
        return true;

    // "nothing to commit" is not a real failure for our purposes.
    if (commitError.contains("nothing to commit", Qt::CaseInsensitive))
        return true;

    if (error) *error = commitError;
    return false;
}

bool GitRunner::forcePushMaster(const QString &dir, QString *error)
{
    QString out;
    return run(dir, {"push", "-f", "origin", "HEAD:master"}, &out, error);
}

bool GitRunner::pull(const QString &dir, QString *error)
{
    QString out;
    return run(dir, {"pull", "origin", "master", "--allow-unrelated-histories"}, &out, error);
}

bool GitRunner::clone(const QString &url, const QString &destDir, QString *error)
{
    QString out;
    return run(QString(), {"clone", url, destDir}, &out, error);
}

void GitRunner::ensureGitIgnore(const QString &dir)
{
    QFile file(QDir(dir).filePath(".gitignore"));
    if (file.exists())
        return;

    if (file.open(QIODevice::WriteOnly)) {
        file.write(
            "# Only the quick-launch data is synced to the remote repo —\n"
            "# the app's own binaries and build output are never committed.\n"
            "*\n"
            "!Game/\n"
            "!Game/**\n"
            "!ThumbNail/\n"
            "!ThumbNail/**\n"
            "!.gitignore\n");
        file.close();
    }
}
