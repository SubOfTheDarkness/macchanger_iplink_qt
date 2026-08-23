#pragma once

#include <QWidget>
#include <QLabel>
#include <QSettings>
#include <QSystemTrayIcon>

namespace Ui {
    class macchanger_widget;
}

class MacChangerWidget : public QWidget {
    Q_OBJECT

public:
    explicit MacChangerWidget(bool hasTraySupport, bool startInTray, QWidget *parent = nullptr);
    ~MacChangerWidget();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void handleCtrlT();
    void handleCtrlR();
    void handleCtrlShiftR();
    void handleCtrlShiftW();
    void handleCtrlEnter();

private:
    const QString APP_VERSION = "v1.0.0";

    Ui::macchanger_widget *ui;

    QLabel *statusBarLabel;

    QString externalConfigPath;
    QString sectionName; 
    QSettings *settings;
    QSystemTrayIcon *trayIcon;

    bool m_hasTraySupport; 

    void createNewPingTab();
    void closePingTab(int index);

    void initPaths();
    void initTray();
    void initConnections();
    void initShortcuts();
    void loadConfig();
    void loadSystemInterfaces();
    
    void updateProfileMac(const QString &profileName);
    void updateCurrentMac(const QString &interface);
    void showInterfaceInfo();
    void reloadConfigAction();
    void setNativeMac();
    void applyMacChange();
};
