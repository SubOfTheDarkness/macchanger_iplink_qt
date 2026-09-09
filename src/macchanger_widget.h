#pragma once

#include <QWidget>
#include <QLabel>
#include <QSettings>
#include <QRegularExpression>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QMap>
#include <qobject.h>
#include "scan_worker.h"

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
    void onAutostartToggled(bool checked);
    void showAboutDialog();
    void showHelpDialog();
    void handleRandomMac();
    void handleSaveProfile();
    void loadSystemInterfaces();

    void collectNetworkLossAlert(const QString &host, const QString &error);
    void sendCentralizedNotification();
    void unlockNotificationSpamProtection();

    void startNetworkScan();
    void onScanFinished(const QVector<DiscoveredDevice> &devices);
    void saveSelectedAlias();
    void changeMacFromScanner();

    void openConfigAction();
    void exportConfigAction();
    void importConfigAction();

    void openLogsDirAction();
    void deleteOldLogsAction();

private:
    Ui::macchanger_widget *ui;
    QLabel *statusBarLabel;

    QString externalConfigPath;
    QString sectionName; 
    QSettings *settings;
    QSystemTrayIcon *trayIcon;
    bool m_hasTraySupport; 

    QTimer *m_centralNotifyTimer;
    QTimer *m_centralSpamLockTimer;
    QMap<QString, QString> m_alertCache;
    bool m_isCentralNotifyLocked;

    ScanWorker *m_scanWorker = nullptr;

    QString pingLogsPath;

    inline static const QRegularExpression macRegex{"([0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2}"};

    QString generateRandomMac();
    void createNewPingTab();
    void closePingTab(int index);
    void initPaths();
    void initTray();
    void initConnections();
    void initShortcuts();
    void loadConfig();
    void updateProfileMac(const QString &profileName);
    void updateCurrentMac(const QString &interface);
    void showInterfaceInfo();
    void reloadConfigAction();
    void setNativeMac();
    void applyMacChange();
    void setAutoStart(bool enable);
    bool isAutoStartEnabled();
};
