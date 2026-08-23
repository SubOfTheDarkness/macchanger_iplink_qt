#pragma once

#include <QWidget>
#include <QProcess>

namespace Ui {
    class ping_tab;
}

class PingTab : public QWidget {
    Q_OBJECT

public:
    explicit PingTab(QWidget *parent = nullptr);
    ~PingTab();

signals:
    void statusChanged(bool isRunning);

private slots:
    void togglePing();
    void readPingOutput();
    void onOctetChanged(int val);

private:
    Ui::ping_tab *ui;
    QProcess *pingProcess;

    void autoDetectSystemGateway();
};
