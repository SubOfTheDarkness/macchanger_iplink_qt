#pragma once

#include <QWidget>
#include <QLabel>
#include <QSettings>
#include <QRegularExpression>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QMap>

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
    void handleRandomMac();
    void handleSaveProfile();

    void collectNetworkLossAlert(const QString &host, const QString &error);
    void sendCentralizedNotification();
    void unlockNotificationSpamProtection();

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

    inline static const QRegularExpression macRegex{"([0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2}"};

    QString generateRandomMac();
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
    void setAutoStart(bool enable);
    bool isAutoStartEnabled();
};
