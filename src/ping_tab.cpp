#include "ping_tab.h"
#include "ui_ping_tab.h"
#include "ping_logger_dialog.h"
#include <QStyle>
#include <QMessageBox>
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>
#include <QUuid>

PingTab::PingTab(const QString &logsDirectory, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ping_tab)
    , m_sentPackets(0)
    , m_lostPackets(0)
    , m_totalRtt(0.0)
    , m_wasConnected(true)
    , m_lastErrorType("Request timeout")
    , m_userStopped(false)
    , m_isConnectionLost(false)
    , m_logsDir(logsDirectory)
{
    ui->setupUi(this);
    autoDetectSystemGateway();
    pingProcess = new QProcess(this);

    QDir().mkpath(m_logsDir);
    
    QString uniqueId = QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
    QString logPath = QString("%1/ping_tab_%2.log").arg(m_logsDir).arg(uniqueId);
    
    m_logFile.setFileName(logPath);
    if (m_logFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        writeToLog("SYSTEM", "New ping tab session initialized.");
    }

    m_lossTimeoutTimer = new QTimer(this);
    m_lossTimeoutTimer->setSingleShot(true);
    connect(m_lossTimeoutTimer, &QTimer::timeout, this, &PingTab::checkNetworkLossTimeout);

    connect(ui->ping_toggle_btn, &QPushButton::clicked, this, &PingTab::togglePing);
    connect(pingProcess, &QProcess::readyReadStandardOutput, this, &PingTab::readPingOutput);
    connect(pingProcess, &QProcess::readyReadStandardError, this, &PingTab::readPingOutput);

    connect(ui->ping_masc_increase_btn, &QPushButton::clicked, this, [this]() { onOctetChanged(1); });
    connect(ui->ping_masc_decrease_btn, &QPushButton::clicked, this, [this]() { onOctetChanged(-1); });

    connect(ui->ping_log_clear_btn, &QPushButton::clicked, this, &PingTab::clearCurrentLog);

    connect(ui->ping_logger_btn, &QPushButton::clicked, this, [this]() {
        PingLoggerDialog *dialog = new PingLoggerDialog(this, this);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->show();
    });

    ui->ping_graph_switch->setChecked(true);
    connect(ui->ping_graph_switch, &SwitchButton::toggled, this, [this](bool checked) {
        ui->ping_graph_widget->setGraphEnabled(checked);
        if (!checked) ui->ping_graph_widget->clearGraph();
    });

    ui->ping_notify_switch->setChecked(true);

    connect(pingProcess, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this, [this](int exitCode, QProcess::ExitStatus exitStatus) {
        ui->ping_toggle_btn->setEnabled(true);
        ui->ping_toggle_btn->setText("Start Ping");
        ui->ping_toggle_btn->setProperty("state", "default");
        ui->ping_toggle_btn->style()->unpolish(ui->ping_toggle_btn);
        ui->ping_toggle_btn->style()->polish(ui->ping_toggle_btn);

        ui->ping_status_lbl->setText("Status: Ready");
        ui->ping_status_lbl->setProperty("state", "default");
        ui->ping_status_lbl->style()->unpolish(ui->ping_status_lbl);
        ui->ping_status_lbl->style()->polish(ui->ping_status_lbl);
        
        m_lossTimeoutTimer->stop();
        emit statusChanged(false);

        if (!m_userStopped && (exitCode != 0 || exitStatus == QProcess::CrashExit)) {
            QString errorStr = QString::fromUtf8(pingProcess->readAllStandardError()).trimmed();
            if (errorStr.isEmpty()) {
                errorStr = "Unknown network error or invalid host configuration.";
            }
            
            writeToLog("SYSTEM_ERROR", QString("Process crashed or failed. Exit code: %1. Output: %2").arg(exitCode).arg(errorStr));

            QMessageBox::critical(this, "Ping Execution Error", 
                QString("<b>System output:</b><br><font color='#ff4f4f'>%1</font>").arg(errorStr));
        }
    });
}

PingTab::~PingTab() {
    if (pingProcess) {
        pingProcess->disconnect();
        if (pingProcess->state() == QProcess::Running) {
            pingProcess->kill();
            pingProcess->waitForFinished(50);
        }
    }
    if (m_logFile.isOpen()) {
        writeToLog("SYSTEM", "Ping tab session closed.");
        m_logFile.close();
    }
    delete ui;
}

void PingTab::writeToLog(const QString &category, const QString &message) {
    if (!m_logFile.isOpen()) return;
    
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
    QString logLine = QString("[%1] [%2] %3\n").arg(timestamp).arg(category).arg(message);
    
    m_logFile.write(logLine.toUtf8());
    m_logFile.flush();
}

void PingTab::clearCurrentLog() {
    if (!m_logFile.isOpen()) return;
    
    m_logFile.close();
    if (m_logFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        writeToLog("SYSTEM", "Log cleared by user. Resuming session...");
    }
}


QString PingTab::targetHost() const {
    if (pingProcess && (pingProcess->state() == QProcess::Running || pingProcess->state() == QProcess::Starting)) {
        return m_currentActiveHost;
    }
    return ui->ping_ip_entry->text().trimmed();
}

QString PingTab::currentLogFileName() const {
    return QFileInfo(m_logFile.fileName()).fileName();
}

QString PingTab::absoluteLogFilePath() const { 
    return m_logFile.fileName(); 
}

bool PingTab::isPingRunning() const { 
    return pingProcess && (pingProcess->state() == QProcess::Running || pingProcess->state() == QProcess::Starting); 
}

void PingTab::autoDetectSystemGateway() {
    QString gwIp = "0.0.0.0";
    QProcess process;
    process.start("ip", QStringList() << "route" << "show");
    if (process.waitForFinished()) {
        QString output = QString::fromUtf8(process.readAllStandardOutput());
        for (const QString &line : output.split('\n')) {
            if (line.startsWith("default via")) {
                QStringList tokens = line.split(" ");
                if (tokens.size() > 2) { gwIp = tokens.at(2); break; }
            }
        }
    }
    ui->ping_ip_entry->setText(gwIp);
}

void PingTab::onOctetChanged(int val) {
    QString currentIp = ui->ping_ip_entry->text().trimmed();
    QStringList parts = currentIp.split(".");
    if (parts.size() == 4) {
        int thirdOctet = parts.at(2).toInt() + val;
        if (thirdOctet < 0) thirdOctet = 255;
        if (thirdOctet > 255) thirdOctet = 0;
        ui->ping_ip_entry->setText(QString("%1.%2.%3.%4").arg(parts.at(0)).arg(parts.at(1)).arg(QString::number(thirdOctet)).arg(parts.at(3)));
    }
}

void PingTab::togglePing() {
    if (pingProcess->state() == QProcess::Running || pingProcess->state() == QProcess::Starting) {
        ui->ping_toggle_btn->setEnabled(false);
        ui->ping_toggle_btn->setText("Stopping...");
        ui->ping_toggle_btn->setProperty("state", "stopping");
        ui->ping_toggle_btn->style()->unpolish(ui->ping_toggle_btn);
        ui->ping_toggle_btn->style()->polish(ui->ping_toggle_btn);
        
        ui->ping_status_lbl->setText("Status: Stopping process...");
        ui->ping_status_lbl->setProperty("state", "default");
        ui->ping_status_lbl->style()->unpolish(ui->ping_status_lbl);
        ui->ping_status_lbl->style()->polish(ui->ping_status_lbl);

        emit logStopped();
        writeToLog("SYSTEM", "Monitoring paused by user.");
        
        m_userStopped = true;
        pingProcess->terminate();
        if (!pingProcess->waitForFinished(400)) { pingProcess->kill(); }
    } else {
        QString targetIp = ui->ping_ip_entry->text().trimmed();
        if (targetIp.isEmpty()) return;
        m_currentActiveHost = targetIp;
        m_userStopped = false;

        m_sentPackets = 0; m_lostPackets = 0; m_totalRtt = 0.0; m_wasConnected = true;
        m_lastErrorType = "Request timeout";
        m_isConnectionLost = false;

        setCurrentPingDanger(false);
        
        ui->ping_current_entry->setText("--");
        ui->ping_avg_entry->setText("--"); 
        ui->ping_loss_entry->setText("0");
        ui->ping_graph_widget->clearGraph();

        emit logStarted(m_currentActiveHost);
        writeToLog("SYSTEM", QString("Monitoring started for target: %1").arg(m_currentActiveHost));

        pingProcess->start("ping", QStringList() << targetIp);
        
        ui->ping_toggle_btn->setText("Stop Ping");
        ui->ping_toggle_btn->setProperty("state", "active");
        ui->ping_toggle_btn->style()->unpolish(ui->ping_toggle_btn);
        ui->ping_toggle_btn->style()->polish(ui->ping_toggle_btn);
        
        ui->ping_status_lbl->setText(QString("Status: Monitoring %1...").arg(targetIp));
        ui->ping_status_lbl->setProperty("state", "active");
        ui->ping_status_lbl->style()->unpolish(ui->ping_status_lbl);
        ui->ping_status_lbl->style()->polish(ui->ping_status_lbl);

        m_lossTimeoutTimer->start(5000); 
        emit statusChanged(true);
    }
}

void PingTab::readPingOutput() {
    while (pingProcess->canReadLine()) {
        QString line = QString::fromUtf8(pingProcess->readLine()).trimmed();
        if (line.isEmpty()) continue;
        writeToLog("PING", line); 
        parsePingLine(line);
    }
}

void PingTab::parsePingLine(const QString &line) {
    static QRegularExpression rttRegex("time=([0-9\\.]+)");
    static QRegularExpression lossRegex("timeout|unreachable|failed|loss", QRegularExpression::CaseInsensitiveOption);

    QRegularExpressionMatch match = rttRegex.match(line);
    
    if (match.hasMatch()) {
        m_lossTimeoutTimer->start(5000);

        m_sentPackets++;
        double currentRtt = match.captured(1).toDouble();
        m_totalRtt += currentRtt;
        double avgRtt = m_totalRtt / m_sentPackets;

        if (m_isConnectionLost) {
            writeToLog("APP_INFO", QString("Connection restored. First successful response RTT: %1 ms").arg(currentRtt));
            m_isConnectionLost = false;
        }

        m_wasConnected = true;
        setCurrentPingDanger(false);

        ui->ping_current_entry->setText(formatRttValue(currentRtt));
        ui->ping_avg_entry->setText(formatRttValue(avgRtt));
        
        emit logSuccessReceived(currentRtt);
        ui->ping_graph_widget->addRttPoint(currentRtt, false);
    } 
    else if (lossRegex.match(line).hasMatch()) {
        m_isConnectionLost = true;
        setCurrentPingDanger(true);
        m_lostPackets++;
        ui->ping_loss_entry->setText(QString::number(m_lostPackets));
        ui->ping_current_entry->setText("Timeout");
        
        ui->ping_graph_widget->addRttPoint(0.0, true);

        if (line.contains("unreachable", Qt::CaseInsensitive)) {
            m_lastErrorType = "Unreachable";
        } else {
            m_lastErrorType = "Timeout";
        }
        emit logLossDetected(m_lastErrorType);
    }
}

void PingTab::checkNetworkLossTimeout() {
    if (ui->ping_notify_switch->isChecked() && m_wasConnected) {
        m_wasConnected = false;
        m_isConnectionLost = true;
        setCurrentPingDanger(true);
        m_lostPackets++;
        ui->ping_loss_entry->setText(QString::number(m_lostPackets));
        ui->ping_current_entry->setText("Timeout");
        writeToLog("APP_ALERT", "Connection lost detected by internal application timer (no response for 5000ms).");
        emit logInternalTimerTriggered();
        emit networkLossDetected(targetHost(), m_lastErrorType);
    }
}

void PingTab::setCurrentPingDanger(bool isDanger) {
    if (ui->ping_current_entry->property("danger").toBool() != isDanger) {
        ui->ping_current_entry->setProperty("danger", isDanger);
        ui->ping_current_entry->style()->unpolish(ui->ping_current_entry);
        ui->ping_current_entry->style()->polish(ui->ping_current_entry);
        ui->ping_current_entry->update();
    }

    if (isDanger) {
        ui->ping_status_lbl->setText(QString("Status Alert: Connection lost to %1!").arg(targetHost()));
        ui->ping_status_lbl->setProperty("state", "danger");
    } else {
        if (pingProcess->state() == QProcess::Running) {
            ui->ping_status_lbl->setText(QString("Status: Monitoring %1...").arg(targetHost()));
            ui->ping_status_lbl->setProperty("state", "active");
        }
    }
    ui->ping_status_lbl->style()->unpolish(ui->ping_status_lbl);
    ui->ping_status_lbl->style()->polish(ui->ping_status_lbl);
}

QString PingTab::formatRttValue(double rttMs) {
    if (rttMs > 9999.0) {
        double rttSec = rttMs / 1000.0;
        return QString::number(rttSec, 'f', 2) + " s";
    }
    return QString::number(rttMs, 'f', 1) + " ms";
}
