#pragma once

#include <QWidget>

class QLabel;

class SplashScreen : public QWidget
{
    Q_OBJECT
public:
    explicit SplashScreen(QWidget *parent = nullptr);

    // Runs the full "check / install Bloxstrap" sequence synchronously
    // (pumping the event loop so the UI keeps repainting), updating the
    // status label as it goes. Always shows "Checking Bloxstrap" for at
    // least 1 second, as requested. Returns true if Bloxstrap ends up
    // installed and ready.
    bool runBloxstrapCheckAndInstall();

private:
    void setStatus(const QString &text);

    QLabel *m_statusLabel;
};
