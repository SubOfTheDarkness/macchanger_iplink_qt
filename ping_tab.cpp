#include "ping_tab.h"
#include "ui_ping_tab.h"
#include <QStyle>
#include <QMessageBox>

PingTab::PingTab(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ping_tab)
    , m_sentPackets(0)
    , m_lostPackets(0)
    , m_totalRtt(0.0)
    , m_wasConnected(true)
    , m_lastErrorType("Request timeout")
{
    ui->setupUi(this);
    autoDetectSystemGateway();
    pingProcess = new QProcess(this);

    m_lossTimeoutTimer = new QTimer(this);
    m_lossTimeoutTimer->setSingleShot(true);
    connect(m_lossTimeoutTimer, &QTimer::timeout, this, &PingTab::checkNetworkLossTimeout);

    connect(ui->ping_toggle_btn, &QPushButton::clicked, this, &PingTab::togglePing);
    connect(pingProcess, &QProcess::readyReadStandardOutput, this, &PingTab::readPingOutput);
    connect(pingProcess, &QProcess::readyReadStandardError, this, &PingTab::readPingOutput);

    connect(ui->ping_masc_increase_btn, &QPushButton::clicked, this, [this]() { onOctetChanged(1); });
    connect(ui->ping_masc_decrease_btn, &QPushButton::clicked, this, [this]() { onOctetChanged(-1); });

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
        
        m_lossTimeoutTimer->stop();
        emit statusChanged(false);

        if (exitCode != 0 || exitStatus == QProcess::CrashExit) {
            QString errorStr = QString::fromUtf8(pingProcess->readAllStandardError()).trimmed();
            
            if (errorStr.isEmpty()) {
                errorStr = "Unknown network error or invalid host configuration.";
            }

            QMessageBox::critical(this, "Ping Execution Error", 
                QString("<b>Failed to start ping process!</b><br><br>"
                        "System output:<br><font color='#ff4f4f'>%1</font>")
                .arg(errorStr));
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
    delete ui;
}


QString PingTab::targetHost() const {
    return ui->ping_ip_entry->text().trimmed();
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
        pingProcess->terminate();
        if (!pingProcess->waitForFinished(400)) { pingProcess->kill(); }
    } else {
        QString targetIp = ui->ping_ip_entry->text().trimmed();
        if (targetIp.isEmpty()) return;

        m_sentPackets = 0; m_lostPackets = 0; m_totalRtt = 0.0; m_wasConnected = true;
        m_lastErrorType = "Request timeout";
        
        ui->ping_current_entry->setText("--");
        ui->ping_avg_entry->setText("--"); 
        ui->ping_loss_entry->setText("0");
        ui->ping_graph_widget->clearGraph();

        pingProcess->start("ping", QStringList() << targetIp);
        
        ui->ping_toggle_btn->setText("Stop Ping");
        ui->ping_toggle_btn->setProperty("state", "active");
        ui->ping_toggle_btn->style()->unpolish(ui->ping_toggle_btn);
        ui->ping_toggle_btn->style()->polish(ui->ping_toggle_btn);
        
        m_lossTimeoutTimer->start(5000); 
        emit statusChanged(true);
    }
}

void PingTab::readPingOutput() {
    while (pingProcess->canReadLine()) {
        QString line = QString::fromUtf8(pingProcess->readLine()).trimmed();
        if (line.isEmpty()) continue;
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

        m_wasConnected = true;

        ui->ping_current_entry->setText(QString::number(currentRtt, 'f', 1) + " ms");
        ui->ping_avg_entry->setText(QString::number(avgRtt, 'f', 1) + " ms");
        ui->ping_graph_widget->addRttPoint(currentRtt);
    } 
    else if (lossRegex.match(line).hasMatch()) {
        m_lostPackets++;
        ui->ping_loss_entry->setText(QString::number(m_lostPackets));
        ui->ping_current_entry->setText("Timeout");
        ui->ping_graph_widget->addRttPoint(0.0);

        if (line.contains("unreachable", Qt::CaseInsensitive)) {
            m_lastErrorType = "Unreachable";
        } else {
            m_lastErrorType = "Timeout";
        }
    }
}

void PingTab::checkNetworkLossTimeout() {
    if (ui->ping_notify_switch->isChecked() && m_wasConnected) {
        m_wasConnected = false;
        
        emit networkLossDetected(targetHost(), m_lastErrorType);
    }
}
