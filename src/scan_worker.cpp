#include "scan_worker.h"
#include <QProcess>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QNetworkInterface>
#include <QAbstractEventDispatcher>

ScanWorker::ScanWorker(QObject *parent) : QThread(parent) {}

void ScanWorker::run() {
    QString subnetBase = getLocalSubnet();
    if (subnetBase.isEmpty()) {
        emit scanFinished(QVector<DiscoveredDevice>());
        return;
    }

    pingIpRange(subnetBase);

    QVector<DiscoveredDevice> devices = parseSystemArpCache();

    emit scanFinished(devices);
}

QString ScanWorker::getLocalSubnet() {
    for (const QNetworkInterface &iface : QNetworkInterface::allInterfaces()) {
        if (iface.flags().testFlag(QNetworkInterface::IsLoopBack) || 
            !iface.flags().testFlag(QNetworkInterface::IsUp)) {
            continue;
        }

        for (const QNetworkAddressEntry &entry : iface.addressEntries()) {
            QHostAddress ip = entry.ip();
            if (ip.protocol() == QAbstractSocket::IPv4Protocol) {
                QString ipStr = ip.toString();
                int lastDot = ipStr.lastIndexOf('.');
                if (lastDot != -1) {
                    return ipStr.left(lastDot + 1);
                }
            }
        }
    }
    return "";
}

void ScanWorker::pingIpRange(const QString &subnetBase) {
    QString shellCommand = QString(
        "for i in $(seq 1 254); do "
        "ping -c 1 -W 1 %1$1$i > /dev/null 2>&1 & "
        "done; "
        "wait"
    ).arg(subnetBase);

    QProcess proc;
    proc.start("sh", QStringList() << "-c" << shellCommand);
    
    proc.waitForFinished(1500); 
}


QVector<DiscoveredDevice> ScanWorker::parseSystemArpCache() {
    QVector<DiscoveredDevice> result;
    QFile file("/proc/net/arp");
    
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return result;
    }

    QTextStream in(&file);
    QString header = in.readLine();

    static QRegularExpression validMacRegex("^(?!00:00:00:00:00:00)([0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2}$");

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) continue;

        QStringList tokens = line.split(QRegularExpression("\\s+"));
        
        if (tokens.size() > 3) {
            QString ip = tokens.at(0);
            QString mac = tokens.at(3).toUpper();

            if (validMacRegex.match(mac).hasMatch()) {
                DiscoveredDevice dev;
                dev.ip = ip;
                dev.mac = mac;
                result.append(dev);
            }
        }
    }
    
    file.close();
    return result;
}
