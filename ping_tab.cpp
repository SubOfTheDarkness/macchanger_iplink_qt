#include "ping_tab.h"
#include "ui_ping_tab.h"
#include <QProcess>
#include <QRegularExpression>

/* 
 * Конструктор автономной вкладки пинга. Разворачивает интерфейс из файла ui_ping_tab.h, 
 * запускает автоматическое вычисление текущего системного шлюза, создает объект QProcess 
 * и связывает сигналы кликов по кнопкам инкремента/декремента и старта сетевого потока.
 */
PingTab::PingTab(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ping_tab)
{
    ui->setupUi(this);

    autoDetectSystemGateway();

    pingProcess = new QProcess(this);

    connect(ui->ping_toggle_btn, &QPushButton::clicked, this, &PingTab::togglePing);
    connect(pingProcess, &QProcess::readyReadStandardOutput, this, &PingTab::readPingOutput);

    connect(ui->ping_masc_increase_btn, &QPushButton::clicked, this, [this]() { onOctetChanged(1); });
    connect(ui->ping_masc_decrease_btn, &QPushButton::clicked, this, [this]() { onOctetChanged(-1); });
}

/* 
 * Деструктор вкладки. Обеспечивает безопасность закрытия: если процесс утилиты ping 
 * активен в Linux, метод отправляет сигнал SIGKILL и синхронно ожидает до 50 мс остановки потока 
 * в ядре, предотвращая падение приложения с ошибкой уничтожения работающего процесса.
 */
PingTab::~PingTab() {
    if (pingProcess && pingProcess->state() == QProcess::Running) {
        pingProcess->kill();
        pingProcess->waitForFinished(50);
    }
    
    delete ui;
}

/* 
 * Автономно опрашивает глобальную таблицу маршрутизации ОС Linux через утилиту 'ip route show'. 
 * Построчно парсит вывод в поисках основного шлюза по умолчанию ('default via'). 
 * Наденный IP-адрес подставляет в поле ввода. Если сеть недоступна, выставляет 0.0.0.0.
 */
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

/* 
 * Срабатывает при клике на кнопки "+" и "-". Парсит текущую строку IP-адреса, 
 * выделяет исключительно третий октет, переводит его в число и добавляет дельту (1 или -1). 
 * Включает круговую защиту байта (значения удерживаются строго в диапазоне от 0 до 255).
 */
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

/* 
 * Переключает состояние сетевого потока. Если пинг запущен — принудительно останавливает его 
 * и шлет сигнал statusChanged(false). Если остановлен — очищает консоль, считывает целевой IP, 
 * асинхронно запускает системную утилиту 'ping' и шлет сигнал statusChanged(true) главному окну.
 */
void PingTab::togglePing() {
    if (pingProcess->state() == QProcess::Running) {
        pingProcess->kill();
        ui->ping_toggle_btn->setText("Start Ping");
        ui->ping_toggle_btn->setStyleSheet("background-color: #28a745; color: white; font-weight: bold;");
        ui->ping_log_text->append("\n[!] Ping stopped by user.");
        
        emit statusChanged(false);
    } else {
        ui->ping_log_text->clear();
        QString targetIp = ui->ping_ip_entry->text().trimmed();
        ui->ping_log_text->append(QString("[*] Ping started for node: %1...\n").arg(targetIp));
        
        pingProcess->start("ping", QStringList() << targetIp);
        ui->ping_toggle_btn->setText("Stop Ping");
        ui->ping_toggle_btn->setStyleSheet("background-color: #dc3545; color: white; font-weight: bold;");
        
        emit statusChanged(true);
    }
}

/* 
 * Вызывается автоматически, как только в stdout запущенного процесса ping появляются новые данные. 
 * Считывает сырой массив байт, декодирует его в UTF-8 строку и дописывает текст в поле лога терминала.
 */
void PingTab::readPingOutput() {
    QByteArray data = pingProcess->readAllStandardOutput();
    ui->ping_log_text->append(QString::fromUtf8(data).trimmed());
}
