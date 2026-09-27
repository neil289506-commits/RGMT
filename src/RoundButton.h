#pragma once

#include <QPushButton>

// A small circular floating action button, drawn by hand (no icon asset
// needed) with a soft shadow and a hover/press color shift.
class RoundButton : public QPushButton
{
    Q_OBJECT
public:
    explicit RoundButton(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    bool m_hovered = false;
};
