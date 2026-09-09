#include "ping_logger_dialog.h"
#include "ui_ping_logger_dialog.h"
#include "ping_tab.h"
#include <QDesktopServices>
#include <QUrl>
#include <QFileInfo>

PingLoggerDialog::PingLoggerDialog(PingTab *associatedTab, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ping_logger_dialog)
    , m_tab(associatedTab)
{
    ui->setupUi(this);
    
    setWindowTitle(QString("Session Live Logger — %1").arg(m_tab->targetHost()));

    updateInfoLabel(m_tab->isPingRunning(), m_tab->targetHost());

    connect(m_tab, &PingTab::logStarted, this, &PingLoggerDialog::handleLogStarted);
    connect(m_tab, &PingTab::logStopped, this, &PingLoggerDialog::handleLogStopped);
    connect(m_tab, &PingTab::logSuccessReceived, this, &PingLoggerDialog::handleSuccessReceived);
    connect(m_tab, &PingTab::logLossDetected, this, &PingLoggerDialog::handleLossDetected);
    connect(m_tab, &PingTab::logInternalTimerTriggered, this, &PingLoggerDialog::handleInternalTimerTriggered);

    connect(ui->ping_logger_open_file_btn, &QPushButton::clicked, this, &PingLoggerDialog::openRawLogFile);
}

PingLoggerDialog::~PingLoggerDialog() {
    delete ui;
}

void PingLoggerDialog::updateInfoLabel(bool isRunning, const QString &host) {
    if (host.isEmpty() || host == "--") {
        ui->ping_logger_info_lbl->setText("Status: <N/a>");
    } else {
        ui->ping_logger_info_lbl->setText(QString("Pinging: %1 [%2]").arg(host).arg(isRunning ? "Active" : "Paused"));
    }
}

void PingLoggerDialog::openRawLogFile() {
    if (m_tab) {
        QString fullPath = m_tab->absoluteLogFilePath();
        if (QFileInfo::exists(fullPath)) {
            QProcess::startDetached("xdg-open", QStringList() << fullPath);
        }
    }
}

void PingLoggerDialog::handleLogStarted(const QString &host) {
    updateInfoLabel(true, host);
    finalizeCurrentStatistics();
    
    QListWidgetItem *item = new QListWidgetItem("▶️ [SYSTEM] Monitoring session started.", ui->ping_logger_list);
    item->setForeground(QBrush(QColor("#00C3FF")));
    m_lastType = LastEntryType::None;
}

void PingLoggerDialog::handleLogStopped() {
    updateInfoLabel(false, m_tab->targetHost());
    finalizeCurrentStatistics();
    
    QListWidgetItem *item = new QListWidgetItem("⏸️ [SYSTEM] Monitoring paused by user.", ui->ping_logger_list);
    item->setForeground(QBrush(QColor("#888888")));
    m_lastType = LastEntryType::None;
}

void PingLoggerDialog::finalizeCurrentStatistics() {
    m_statPackets = 0;
    m_statSumRtt = 0.0;
    m_statMinRtt = -1.0;
    m_statMaxRtt = 0.0;
}

void PingLoggerDialog::handleSuccessReceived(double rtt) {
    if (m_lastType == LastEntryType::SystemError || m_lastType == LastEntryType::TimerAlert) {
        QListWidgetItem *restoredItem = new QListWidgetItem("✅ Connection restored", ui->ping_logger_list);
        restoredItem->setForeground(QBrush(QColor("#28a745")));
        m_lastType = LastEntryType::ConnectionRestored;
        finalizeCurrentStatistics();
    }

    m_statPackets++;
    m_statSumRtt += rtt;
    if (m_statMinRtt < 0 || rtt < m_statMinRtt) m_statMinRtt = rtt;
    if (rtt > m_statMaxRtt) m_statMaxRtt = rtt;

    double avgRtt = m_statSumRtt / m_statPackets;
    QString statText = QString("📊 %1 packets, Avg: %2 ms, Max: %3 ms, Min: %4 ms")
                       .arg(m_statPackets)
                       .arg(avgRtt, 0, 'f', 1)
                       .arg(m_statMaxRtt, 0, 'f', 1)
                       .arg(m_statMinRtt, 0, 'f', 1);

    if (m_lastType == LastEntryType::Statistics) {
        QListWidgetItem *lastItem = ui->ping_logger_list->item(ui->ping_logger_list->count() - 1);
        if (lastItem) lastItem->setText(statText);
    } else {
        QListWidgetItem *newItem = new QListWidgetItem(statText, ui->ping_logger_list);
        newItem->setForeground(QBrush(QColor("#ffffff")));
        m_lastType = LastEntryType::Statistics;
    }
    
    ui->ping_logger_list->scrollToBottom();
}

void PingLoggerDialog::handleLossDetected(const QString &errorType) {
    QString shortError = QString("Request %1").arg(errorType.toLower());

    if (m_lastType == LastEntryType::SystemError && m_lastErrorText == shortError) {
        m_errorSpamCount++;
        QListWidgetItem *lastItem = ui->ping_logger_list->item(ui->ping_logger_list->count() - 1);
        if (lastItem) {
            lastItem->setText(QString("❌ %1 (x%2)").arg(shortError).arg(m_errorSpamCount));
        }
    } else {
        m_errorSpamCount = 1;
        m_lastErrorText = shortError;
        
        QListWidgetItem *newItem = new QListWidgetItem(QString("❌ %1 (x1)").arg(shortError), ui->ping_logger_list);
        newItem->setForeground(QBrush(QColor("#ff4f4f")));
        m_lastType = LastEntryType::SystemError;
    }
    
    ui->ping_logger_list->scrollToBottom();
}

void PingLoggerDialog::handleInternalTimerTriggered() {
    QListWidgetItem *item = new QListWidgetItem("⚠️ Connection Loss - Timer 5s", ui->ping_logger_list);
    item->setForeground(QBrush(QColor("#ff9f43")));
    m_lastType = LastEntryType::TimerAlert;
    
    ui->ping_logger_list->scrollToBottom();
}
