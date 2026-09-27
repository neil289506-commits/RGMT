#include "Theme.h"

#include <QApplication>
#include <QGuiApplication>
#include <QStyleHints>
#include <QString>

namespace {

// Roblox-red accent, used sparingly for primary actions (the "+" button,
// the "Play" button) so the app reads as "Roblox tooling" without copying
// any actual Roblox UI.
constexpr const char *kAccent = "#E2241D";
constexpr const char *kAccentHover = "#F13B33";
constexpr const char *kAccentPressed = "#C21B15";

QString lightStyleSheet()
{
    return QString(R"(
        QWidget { background-color: #F4F5F7; color: #1F2328; font-size: 10pt; }
        QScrollArea { background: transparent; }
        QScrollArea > QWidget > QWidget { background: transparent; }
        QLabel#subtitleLabel { color: #6B7280; }
        QLineEdit, QProgressBar {
            background-color: #FFFFFF; border: 1px solid #D8DAE0;
            border-radius: 6px; padding: 6px 8px;
        }
        QLineEdit:focus { border: 1px solid %1; }
        QDialog { background-color: #F4F5F7; }
        QPushButton {
            background-color: #FFFFFF; border: 1px solid #D8DAE0;
            border-radius: 6px; padding: 6px 14px;
        }
        QPushButton:hover { border-color: %1; color: %1; }
        QPushButton:pressed { background-color: #ECEDF0; }
        QPushButton#primaryButton {
            background-color: %1; border: none; color: white; font-weight: 600;
        }
        QPushButton#primaryButton:hover { background-color: %2; }
        QPushButton#primaryButton:pressed { background-color: %3; }
    )").arg(kAccent, kAccentHover, kAccentPressed);
}

QString darkStyleSheet()
{
    return QString(R"(
        QWidget { background-color: #1E1F22; color: #F2F3F5; font-size: 10pt; }
        QScrollArea { background: transparent; }
        QScrollArea > QWidget > QWidget { background: transparent; }
        QLabel#subtitleLabel { color: #9AA0A8; }
        QLineEdit, QProgressBar {
            background-color: #2B2D31; border: 1px solid #3A3C42;
            border-radius: 6px; padding: 6px 8px; color: #F2F3F5;
        }
        QLineEdit:focus { border: 1px solid %1; }
        QDialog { background-color: #1E1F22; }
        QPushButton {
            background-color: #2B2D31; border: 1px solid #3A3C42;
            border-radius: 6px; padding: 6px 14px; color: #F2F3F5;
        }
        QPushButton:hover { border-color: %1; color: %1; }
        QPushButton:pressed { background-color: #232428; }
        QPushButton#primaryButton {
            background-color: %1; border: none; color: white; font-weight: 600;
        }
        QPushButton#primaryButton:hover { background-color: %2; }
        QPushButton#primaryButton:pressed { background-color: %3; }
    )").arg(kAccent, kAccentHover, kAccentPressed);
}

void applyForCurrentScheme(QApplication &app)
{
    const bool dark = QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;
    app.setStyleSheet(dark ? darkStyleSheet() : lightStyleSheet());
}

} // namespace

namespace Theme {

void apply(QApplication &app)
{
    applyForCurrentScheme(app);

    QObject::connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged,
                      &app, [&app](Qt::ColorScheme) { applyForCurrentScheme(app); });
}

} // namespace Theme
