#pragma once

#include <QDialog>
#include <QListWidgetItem>

namespace Ui {
    class ping_logger_dialog;
}

class PingTab;

class PingLoggerDialog : public QDialog {
    Q_OBJECT

public:
    explicit PingLoggerDialog(PingTab *associatedTab, QWidget *parent = nullptr);
    ~PingLoggerDialog();

private slots:
    void handleLogStarted(const QString &host);
    void handleLogStopped();
    void handleSuccessReceived(double rtt);
    void handleLossDetected(const QString &errorType);
    void handleInternalTimerTriggered();
    void openRawLogFile();

private:
    Ui::ping_logger_dialog *ui;
    PingTab *m_tab;

    enum class LastEntryType {
        None,
        Statistics,
        SystemError,
        TimerAlert,
        ConnectionRestored
    };

    LastEntryType m_lastType = LastEntryType::None;
    QString m_lastErrorText;

    int m_statPackets = 0;
    double m_statSumRtt = 0.0;
    double m_statMinRtt = -1.0;
    double m_statMaxRtt = 0.0;

    int m_errorSpamCount = 0;

    void updateInfoLabel(bool isRunning, const QString &host);
    void finalizeCurrentStatistics();
};
