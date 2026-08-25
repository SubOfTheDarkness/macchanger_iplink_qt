#include "ping_tab.h"
#include "ui_ping_tab.h"
#include <QProcess>
#include <QStyle>
#include <QRegularExpression>

PingTab::PingTab(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ping_tab)
{
    ui->setupUi(this);

    autoDetectSystemGateway();

    pingProcess = new QProcess(this);

    connect(ui->ping_toggle_btn, &QPushButton::clicked, this, &PingTab::togglePing);
    connect(pingProcess, &QProcess::readyReadStandardOutput, this, &PingTab::readPingOutput);
    connect(pingProcess, &QProcess::readyReadStandardError, this, &PingTab::readPingOutput);

    connect(pingProcess, &QProcess::finished, this, [this]() {
        ui->ping_toggle_btn->setEnabled(true);
        ui->ping_toggle_btn->setText("Start Ping");
        
        ui->ping_toggle_btn->setProperty("state", "default");
        ui->ping_toggle_btn->style()->unpolish(ui->ping_toggle_btn);
        ui->ping_toggle_btn->style()->polish(ui->ping_toggle_btn);
        
        emit statusChanged(false);
    });

    connect(ui->ping_masc_increase_btn, &QPushButton::clicked, this, [this]() { onOctetChanged(1); });
    connect(ui->ping_masc_decrease_btn, &QPushButton::clicked, this, [this]() { onOctetChanged(-1); });
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

void PingTab::autoDetectSystemGateway() {
    QString gwIp = "0.0.0.0";
    QProcess process;
    process.start("ip", QStringList() << "route" << "show");
    
    if (process.waitForFinished()) {
        QString output = QString::fromUtf8(process.readAllStandardOutput());
        for (const QString &line : output.split('\n')) {
            if (line.startsWith("default via")) {
                QStringList tokens = line.split(" ");
                if (tokens.size() > 2) {
                    gwIp = tokens.at(2);
                    break;
                }
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
        
        ui->ping_ip_entry->setText(QString("%1.%2.%3.%4")
                                   .arg(parts.at(0))
                                   .arg(parts.at(1))
                                   .arg(QString::number(thirdOctet))
                                   .arg(parts.at(3)));
    }
}

void PingTab::togglePing() {
    if (pingProcess->state() == QProcess::Running || pingProcess->state() == QProcess::Starting) {
        ui->ping_toggle_btn->setEnabled(false);
        ui->ping_toggle_btn->setText("Stopping...");
        
        ui->ping_toggle_btn->setProperty("state", "stopping");
        ui->ping_toggle_btn->style()->unpolish(ui->ping_toggle_btn);
        ui->ping_toggle_btn->style()->polish(ui->ping_toggle_btn);
        
        ui->ping_log_text->append("<br><span style='color: #ff7675;'>[!] Terminating background ping...</span>");
        
        pingProcess->terminate();
        if (!pingProcess->waitForFinished(400)) {
            pingProcess->kill();
        }
        ui->ping_log_text->append("<br><span style='color: #ff4f4f;'>[!] Ping terminated.</span>");
    } else {
        ui->ping_log_text->clear();
        QString targetIp = ui->ping_ip_entry->text().trimmed();
        
        if (targetIp.isEmpty()) {
            ui->ping_log_text->append("<span style='color: #ff003c;'>[ERROR] Target IP is empty!</span>");
            return;
        }

        ui->ping_log_text->append(QString("<span style='color: #00C3FF;'>[*] Ping started for node: %1...</span><br>").arg(targetIp));
        
        pingProcess->start("ping", QStringList() << targetIp);
        
        ui->ping_toggle_btn->setText("Stop Ping");
        ui->ping_toggle_btn->setProperty("state", "active");
        ui->ping_toggle_btn->style()->unpolish(ui->ping_toggle_btn);
        ui->ping_toggle_btn->style()->polish(ui->ping_toggle_btn);
        
        emit statusChanged(true);
    }
}

void PingTab::readPingOutput() {
    QByteArray stdOut = pingProcess->readAllStandardOutput();
    QByteArray stdErr = pingProcess->readAllStandardError();
    
    if (!stdOut.isEmpty()) {
        ui->ping_log_text->append(QString::fromUtf8(stdOut).trimmed());
    }
    if (!stdErr.isEmpty()) {
        ui->ping_log_text->append(QString("<span style='color: #ff7675;'>%1</span>")
                                  .arg(QString::fromUtf8(stdErr).trimmed()));
    }
}
