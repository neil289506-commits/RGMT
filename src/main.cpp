#include <QApplication>

#include "MainWindow.h"
#include "SplashScreen.h"
#include "Theme.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Roblox Game Maintenance Tool");
    app.setOrganizationName("Neil");
    Theme::apply(app);

    SplashScreen splash;
    splash.show();
    app.processEvents();

    const bool bloxstrapReady = splash.runBloxstrapCheckAndInstall();
    if (!bloxstrapReady)
        return 1;

    MainWindow window;
    window.show();
    splash.close();

    return app.exec();
}
