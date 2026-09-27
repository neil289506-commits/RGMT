#include "SplashScreen.h"

#include "BloxstrapManager.h"

#include <QElapsedTimer>
#include <QEventLoop>
#include <QFont>
#include <QGuiApplication>
#include <QLabel>
#include <QMessageBox>
#include <QScreen>
#include <QTimer>
#include <QVBoxLayout>

SplashScreen::SplashScreen(QWidget *parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::SplashScreen);
    setFixedSize(360, 140);

    auto *layout = new QVBoxLayout(this);
    m_statusLabel = new QLabel(tr("Checking Bloxstrap"), this);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    QFont font = m_statusLabel->font();
    font.setPointSize(font.pointSize() + 2);
    m_statusLabel->setFont(font);
    layout->addWidget(m_statusLabel);

    if (QScreen *screen = QGuiApplication::primaryScreen()) {
        const QRect screenGeometry = screen->geometry();
        move(screenGeometry.center() - rect().center());
    }
}

void SplashScreen::setStatus(const QString &text)
{
    m_statusLabel->setText(text);
    repaint();
}

static void pumpEventLoopFor(int milliseconds)
{
    QEventLoop loop;
    QTimer::singleShot(milliseconds, &loop, &QEventLoop::quit);
    loop.exec();
}

bool SplashScreen::runBloxstrapCheckAndInstall()
{
    QElapsedTimer timer;
    timer.start();

    setStatus(tr("Checking Bloxstrap"));

    const bool alreadyInstalled = BloxstrapManager::isInstalled();

    // Keep the "Checking Bloxstrap" message on screen for at least 1 second,
    // regardless of how fast the detection itself finished.
    const qint64 elapsed = timer.elapsed();
    if (elapsed < 1000)
        pumpEventLoopFor(int(1000 - elapsed));

    if (alreadyInstalled)
        return true;

    setStatus(tr("Installing Bloxstrap..."));

    QString error;
    const bool installed = BloxstrapManager::installViaWinget(&error);

    if (!installed) {
        QMessageBox::critical(this, tr("安裝失敗"),
            tr("無法自動安裝 Bloxstrap：\n%1\n\n請手動安裝後重新開啟本程式。").arg(error));
        return false;
    }

    setStatus(tr("Bloxstrap installed"));
    pumpEventLoopFor(500);
    return true;
}
