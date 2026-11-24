#include "MainWindow.h"

#include <QApplication>
#include <QPalette>
#include <QStyleFactory>

static void applyDarkTheme(QApplication &app)
{
    // Fusion + an explicit palette gives a consistent dark look on every platform.
    app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));

    QPalette p;
    const QColor window(30, 32, 40), base(22, 24, 31), button(45, 49, 63);
    const QColor text(226, 229, 236), disabled(120, 125, 140), accent(64, 140, 255);

    p.setColor(QPalette::Window, window);
    p.setColor(QPalette::WindowText, text);
    p.setColor(QPalette::Base, base);
    p.setColor(QPalette::AlternateBase, window);
    p.setColor(QPalette::Text, text);
    p.setColor(QPalette::Button, button);
    p.setColor(QPalette::ButtonText, text);
    p.setColor(QPalette::BrightText, Qt::red);
    p.setColor(QPalette::Highlight, accent);
    p.setColor(QPalette::HighlightedText, Qt::white);
    p.setColor(QPalette::ToolTipBase, QColor(50, 54, 70));
    p.setColor(QPalette::ToolTipText, text);
    p.setColor(QPalette::PlaceholderText, disabled);
    p.setColor(QPalette::Disabled, QPalette::Text, disabled);
    p.setColor(QPalette::Disabled, QPalette::ButtonText, disabled);
    p.setColor(QPalette::Disabled, QPalette::WindowText, disabled);
    app.setPalette(p);

    app.setStyleSheet(QStringLiteral(
        "QGroupBox { border: 1px solid #3a3f52; border-radius: 6px; margin-top: 14px; padding-top: 8px; font-weight: 600; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; color: #9db8ff; }"
        "QPushButton { background: #2f3447; border: 1px solid #444b63; border-radius: 5px; padding: 7px 10px; }"
        "QPushButton:hover { background: #3a415a; }"
        "QPushButton:pressed { background: #262b3b; }"
        "QPushButton:disabled { color: #70768a; background: #262a38; border-color: #333849; }"
        "QPushButton#startButton:enabled { background: #1f7a4a; border-color: #2a9d60; font-weight: 600; }"
        "QPushButton#startButton:enabled:hover { background: #26935a; }"
        "QToolTip { color: #e2e5ec; background: #323646; border: 1px solid #4a516a; }"));
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("SimuLab"));
    QApplication::setApplicationVersion(QStringLiteral("1.0.0"));

    applyDarkTheme(app);

    MainWindow window;
    window.show();
    return app.exec();
}
