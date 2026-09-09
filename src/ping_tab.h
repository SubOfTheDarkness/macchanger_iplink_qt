#pragma once

#include <QWidget>
#include <QProcess>
#include <QTimer>
#include <QRegularExpression>
#include <QFile>

namespace Ui {
    class ping_tab;
}

class PingTab : public QWidget {
    Q_OBJECT

public:
    explicit PingTab(const QString &logsDirectory, QWidget *parent = nullptr);
    ~PingTab();

    QString targetHost() const;
    QString currentLogFileName() const;
    QString absoluteLogFilePath() const;
    bool isPingRunning() const;

signals:
    void statusChanged(bool isRunning);
    void networkLossDetected(const QString &host, const QString &error);

    void logStarted(const QString &host);
    void logStopped();
    void logSuccessReceived(double rtt);
    void logLossDetected(const QString &errorType);
    void logInternalTimerTriggered();

private slots:
    void togglePing();
    void readPingOutput();
    void onOctetChanged(int val);
    void checkNetworkLossTimeout();
    void clearCurrentLog();

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

    QString m_currentActiveHost;
    
    QString m_logsDir;
    QFile m_logFile;
    bool m_isConnectionLost;
    void writeToLog(const QString &category, const QString &message);

    QString formatRttValue(double rttMs);
    void autoDetectSystemGateway();
    void parsePingLine(const QString &line);
    void setCurrentPingDanger(bool isDanger);
};
