#pragma once

#include <QWidget>
#include <QProcess>
#include <QTimer>
#include <QRegularExpression>

namespace Ui {
    class ping_tab;
}

class PingTab : public QWidget {
    Q_OBJECT

public:
    explicit PingTab(QWidget *parent = nullptr);
    ~PingTab();

    QString targetHost() const;

signals:
    void statusChanged(bool isRunning);
    void networkLossDetected(const QString &host, const QString &error);

private slots:
    void togglePing();
    void readPingOutput();
    void onOctetChanged(int val);
    void checkNetworkLossTimeout();

private:
    Ui::ping_tab *ui;
    QProcess *pingProcess;
    bool m_userStopped;

    int m_sentPackets;
    int m_lostPackets;
    double m_totalRtt;

    QTimer *m_lossTimeoutTimer;
    bool m_wasConnected;
    QString m_lastErrorType;

    void autoDetectSystemGateway();
    void parsePingLine(const QString &line);

    void setCurrentPingDanger(bool isDanger);
};
