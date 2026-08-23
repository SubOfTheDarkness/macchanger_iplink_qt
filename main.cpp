#include <QApplication>
#include <QCommandLineParser>
#include <QSystemTrayIcon>
#include "macchanger_widget.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    QApplication::setQuitOnLastWindowClosed(false); 
    
    QCommandLineParser parser;
    QCommandLineOption trayOption("tray", "Start minimized to system tray");
    parser.addOption(trayOption);
    parser.process(app);

    bool isTraySupported = QSystemTrayIcon::isSystemTrayAvailable();
    
    bool startInTray = parser.isSet(trayOption) && isTraySupported;
    
    if (!isTraySupported) {
        QApplication::setQuitOnLastWindowClosed(true);
    }
    
    MacChangerWidget window(isTraySupported, startInTray);
    
    return app.exec();
}
