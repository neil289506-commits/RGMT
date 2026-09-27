#pragma once

class QApplication;

// Applies a light/dark-aware QSS theme to the whole app, and keeps it in
// sync if the OS color scheme changes while the app is running.
namespace Theme
{
    void apply(QApplication &app);
}
