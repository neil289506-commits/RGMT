#include "BloxstrapManager.h"

#include <QDir>
#include <QEventLoop>
#include <QFileInfo>
#include <QProcess>
#include <QSettings>
#include <QTimer>

bool BloxstrapManager::checkRegistryUninstallEntry()
{
    // Bloxstrap (Velopack-based) registers a per-user uninstall entry under:
    // HKCU\Software\Microsoft\Windows\CurrentVersion\Uninstall\Bloxstrap
    QSettings uninstall(
        "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Bloxstrap",
        QSettings::NativeFormat);
    const QString displayName = uninstall.value("DisplayName").toString();
    return !displayName.isEmpty() && displayName.contains("Bloxstrap", Qt::CaseInsensitive);
}

bool BloxstrapManager::checkLocalAppDataFolder()
{
    // %LocalAppData%\Bloxstrap\Bloxstrap.exe
    const QString appData = qEnvironmentVariable("LOCALAPPDATA");
    if (appData.isEmpty())
        return false;

    const QString exePath = QDir(appData).filePath("Bloxstrap/Bloxstrap.exe");
    return QFileInfo::exists(exePath);
}

bool BloxstrapManager::checkProtocolHandler()
{
    // Bloxstrap installs itself as the handler for the roblox-player
    // protocol (the same protocol the official Roblox launcher uses),
    // pointing its command at Bloxstrap.exe. Note: this is only registered
    // after Bloxstrap has actually been run once, not merely installed.
    QSettings protocolHandler(
        "HKEY_CURRENT_USER\\Software\\Classes\\roblox-player\\shell\\open\\command",
        QSettings::NativeFormat);
    const QString command = protocolHandler.value(".").toString();
    return command.contains("Bloxstrap", Qt::CaseInsensitive);
}

bool BloxstrapManager::isInstalled()
{
    return checkRegistryUninstallEntry()
        || checkLocalAppDataFolder()
        || checkProtocolHandler();
}

QString BloxstrapManager::installedExecutablePath()
{
    const QString appData = qEnvironmentVariable("LOCALAPPDATA");
    if (!appData.isEmpty()) {
        const QString exePath = QDir(appData).filePath("Bloxstrap/Bloxstrap.exe");
        if (QFileInfo::exists(exePath))
            return exePath;
    }

    QSettings protocolHandler(
        "HKEY_CURRENT_USER\\Software\\Classes\\roblox-player\\shell\\open\\command",
        QSettings::NativeFormat);
    const QString command = protocolHandler.value(".").toString();
    if (command.contains("Bloxstrap", Qt::CaseInsensitive)) {
        // Command is usually of the form: "C:\...\Bloxstrap.exe" "%1"
        if (command.startsWith('"')) {
            const int end = command.indexOf('"', 1);
            if (end > 0)
                return command.mid(1, end - 1);
        }
        return command;
    }

    return QString();
}

void BloxstrapManager::registerAsProtocolHandler()
{
    const QString exePath = installedExecutablePath();
    if (exePath.isEmpty())
        return;

    // Per the Bloxstrap wiki, simply running Bloxstrap once is what makes
    // it register itself as the roblox-player protocol handler. Launch it
    // detached (not waited on) so it can do its first-run setup; we don't
    // need to block on it finishing.
    QProcess::startDetached(exePath, {});

    // Give it a brief moment to write the registry key before we move on.
    QEventLoop loop;
    QTimer::singleShot(1500, &loop, &QEventLoop::quit);
    loop.exec();
}

bool BloxstrapManager::installViaWinget(QString *errorOut)
{
    QProcess process;
    process.setProgram("winget");
    process.setArguments({"install", "-e", "--id", "pizzaboxer.Bloxstrap",
                           "--accept-source-agreements", "--accept-package-agreements"});
    process.start();

    if (!process.waitForStarted(5000)) {
        if (errorOut)
            *errorOut = QObject::tr("無法啟動 winget，請確認已安裝 App Installer。");
        return false;
    }

    // Pump the event loop while winget runs so the splash screen stays
    // responsive instead of appearing frozen.
    QEventLoop loop;
    QObject::connect(&process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                      &loop, &QEventLoop::quit);
    loop.exec();

    const bool success = (process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0);
    if (!success) {
        if (errorOut) {
            *errorOut = QString::fromLocal8Bit(process.readAllStandardError());
            if (errorOut->isEmpty())
                *errorOut = QObject::tr("winget 安裝失敗（結束代碼 %1）").arg(process.exitCode());
        }
        return false;
    }

    registerAsProtocolHandler();
    return true;
}
