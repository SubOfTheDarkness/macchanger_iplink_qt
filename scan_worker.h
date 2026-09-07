#pragma once

#include <QThread>
#include <QVector>
#include <QString>
#include <QPair>

struct DiscoveredDevice {
    QString ip;
    QString mac;
};

class ScanWorker : public QThread {
    Q_OBJECT

public:
    explicit ScanWorker(QObject *parent = nullptr);

signals:
    void scanFinished(const QVector<DiscoveredDevice> &devices);

protected:
    void run() override;

private:
    QString getLocalSubnet();
    void pingIpRange(const QString &subnetBase);
    QVector<DiscoveredDevice> parseSystemArpCache();
};
