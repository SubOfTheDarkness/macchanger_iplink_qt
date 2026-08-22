#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QRegularExpression>
#include <QPushButton>
#include <QMessageBox>
#include <QSettings>
#include <QProcess>
#include <QDir>
#include <QFileInfo>
#include <QTabWidget>
#include <QSpinBox>
#include <QTextEdit>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QCloseEvent>
#include <QStyle>
#include <QCommandLineParser>
#include <QTextStream>

class MacChangerWidget : public QWidget {
public:
    /* Конструктор окна. Инитит конфиг, создает иконку в трее, рисует окно и загружает в интерфейс параметры из конфига */
    MacChangerWidget(QWidget *parent = nullptr) : QWidget(parent) {
        sectionName = "MAC_ALIASES"; 

        initPaths();
        initTray();
        initLayout();
        loadConfig();
        loadSystemInterfaces();
        
        updateCurrentMac(comboInterfaces->currentText());
        updateProfileMac(comboProfiles->currentText());
        autoDetectGateway();
    }
    /* Деструктор. Завершает процесс пинга если он запущен(чтобы в системе не висел ненужный никому неубитый процесс) */
    ~MacChangerWidget() {
        if (pingProcess && pingProcess->state() == QProcess::Running) {
            pingProcess->terminate();
            if (!pingProcess->waitForFinished(1000)) {
                pingProcess->kill();
            }
        }
    }

protected:
    /* Перехват закрытия окна, чтобы окно не закрылось а свернулось в трей */
    void closeEvent(QCloseEvent *event) override {
        if (trayIcon->isVisible()) {
            this->hide();
            event->ignore();
        }
    }

private:
    QString externalConfigPath;
    QString sectionName; 
    QSettings *settings;
    QSystemTrayIcon *trayIcon;

    QComboBox *comboInterfaces;
    QLineEdit *labelCurrentMac;
    QTextEdit *txtInterfaceInfo;
    QComboBox *comboProfiles;
    QLineEdit *labelProfileMac; 
    QLineEdit *editCustomMac;
    QPushButton *btnApply;

    QLineEdit *editCustomIp;
    QSpinBox *spinThirdOctet;
    QTextEdit *txtPingOutput;
    QProcess *pingProcess;
    QPushButton *btnStartPing;

    /* Загрузка конфига и создание дефолта. Создает файл по указанному пути с тем что было в дефолте, если что - грузит только дефолт. */
    void initPaths() {
        externalConfigPath = QDir::homePath() + "/.config/macchanger/address_aliases.ini";
        
        QFileInfo fileInfo(externalConfigPath);
        if (!fileInfo.exists()) {
            QDir().mkpath(fileInfo.absolutePath());
            QFile file(externalConfigPath);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream out(&file);
                out << "; Внешний конфигурационный файл. Переопределяет дефолты.\n";
                out << "[" << sectionName << "]\n";
                file.close();
            }
        }

        settings = new QSettings(externalConfigPath, QSettings::IniFormat, this);
        
        QSettings fallbackSettings(":/default_config.ini", QSettings::IniFormat);
        
        for (const QString &group : fallbackSettings.childGroups()) {
            fallbackSettings.beginGroup(group);
            settings->beginGroup(group);
            for (const QString &key : fallbackSettings.allKeys()) {
                if (!settings->contains(key)) {
                    settings->setValue(key, fallbackSettings.value(key));
                }
            }
            settings->endGroup();
            fallbackSettings.endGroup();
        }
    }

    /* Инициализация иконки в трее */
    void initTray() {
        trayIcon = new QSystemTrayIcon(this);
        trayIcon->setIcon(style()->standardIcon(QStyle::SP_DriveNetIcon));
        trayIcon->setToolTip("MAC Changer Tool");

        QMenu *trayMenu = new QMenu(this);
        QAction *actShow = trayMenu->addAction("Развернуть");
        
        QAction *actRestart = trayMenu->addAction("Перезапустить приложение");
        
        QAction *actExit = trayMenu->addAction("Выход");

        trayIcon->setContextMenu(trayMenu);
        trayIcon->show();

        connect(actShow, &QAction::triggered, this, &QWidget::showNormal);
        connect(actExit, &QAction::triggered, []() { qApp->exit(); });

        connect(actRestart, &QAction::triggered, []() {
            QProcess::startDetached(QCoreApplication::applicationFilePath(), QCoreApplication::arguments());
            qApp->quit();
        });

        connect(trayIcon, &QSystemTrayIcon::activated, [this](QSystemTrayIcon::ActivationReason reason){
            if (reason == QSystemTrayIcon::Trigger) {
                if (this->isVisible()) this->hide();
                else this->showNormal();
            }
        });
    }


    /* Рисование окна */
    void initLayout() {
        setWindowTitle("MAC Tool (Qt6)");
        setMinimumSize(450, 450);
        setWindowIcon(style()->standardIcon(QStyle::SP_DriveNetIcon));

        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        QTabWidget *tabWidget = new QTabWidget(this);
        QString roStyle = "background-color: #e0e0e0; color: #000000; font-weight: bold; padding: 4px; border: 1px solid #b0b0b0; border-radius: 3px;";

        QWidget *macTab = new QWidget(this);
        QVBoxLayout *macLayout = new QVBoxLayout(macTab);

        QHBoxLayout *horizSplitter = new QHBoxLayout();

        QVBoxLayout *leftColumn = new QVBoxLayout();
        
        leftColumn->addWidget(new QLabel("1. Сетевой интерфейс:"));
        comboInterfaces = new QComboBox(this);
        leftColumn->addWidget(comboInterfaces);

        leftColumn->addWidget(new QLabel("Текущий MAC в системе:"));
        labelCurrentMac = new QLineEdit(this);
        labelCurrentMac->setReadOnly(true);
        labelCurrentMac->setStyleSheet(roStyle);
        leftColumn->addWidget(labelCurrentMac);

        leftColumn->addSpacing(5); 
        QPushButton *btnRestoreDefault = new QPushButton("Родной MAC", this);
        btnRestoreDefault->setStyleSheet("background-color: #6c757d; color: white; font-weight: bold; padding: 4px;");
        leftColumn->addWidget(btnRestoreDefault);

        QVBoxLayout *rightColumn = new QVBoxLayout();
        
        rightColumn->addWidget(new QLabel("2. Профиль для смены:"));
        comboProfiles = new QComboBox(this);
        rightColumn->addWidget(comboProfiles);

        QPushButton *btnReloadConfig = new QPushButton("Обновить конфиг", this);
        btnReloadConfig->setStyleSheet("background-color: #e17055; color: white; font-weight: bold; padding: 4px;");
        rightColumn->addWidget(btnReloadConfig);

        rightColumn->addWidget(new QLabel("MAC-адрес профиля:"));
        labelProfileMac = new QLineEdit(this);
        labelProfileMac->setReadOnly(true);
        labelProfileMac->setStyleSheet(roStyle);
        rightColumn->addWidget(labelProfileMac);

        editCustomMac = new QLineEdit(this);
        editCustomMac->setInputMask("HH:HH:HH:HH:HH:HH;_");
        editCustomMac->setPlaceholderText("AA:BB:CC:DD:EE:FF");
        editCustomMac->hide();
        rightColumn->addWidget(editCustomMac);

        horizSplitter->addLayout(leftColumn, 1);
        horizSplitter->addLayout(rightColumn, 1);
        macLayout->addLayout(horizSplitter);

        macLayout->addWidget(new QLabel("Подробная информация об интерфейсе:"));
        txtInterfaceInfo = new QTextEdit(this);
        txtInterfaceInfo->setReadOnly(true);
        txtInterfaceInfo->setMaximumHeight(90);
        txtInterfaceInfo->setStyleSheet("background-color: #2d3436; color: #00cec9; font-family: monospace; font-size: 11px;");
        macLayout->addWidget(txtInterfaceInfo);
        macLayout->addStretch(); 

        macLayout->addSpacing(10);
        btnApply = new QPushButton("Применить изменения", this);
        btnApply->setStyleSheet("background-color: #007acc; color: white; font-weight: bold; padding: 8px;");
        macLayout->addWidget(btnApply);

        QWidget *pingTab = new QWidget(this);
        QVBoxLayout *pingLayout = new QVBoxLayout(pingTab);

        pingLayout->addWidget(new QLabel("Быстрая смена подсети (192.168.X.1):"));
        QHBoxLayout *octetLayout = new QHBoxLayout();
        spinThirdOctet = new QSpinBox(this);
        spinThirdOctet->setRange(0, 255);
        spinThirdOctet->setValue(0);
        octetLayout->addWidget(new QLabel("X = "));
        octetLayout->addWidget(spinThirdOctet);
        pingLayout->addLayout(octetLayout);

        pingLayout->addWidget(new QLabel("Целевой IP-адрес для пинга:"));
        editCustomIp = new QLineEdit("192.168.0.1", this);
        pingLayout->addWidget(editCustomIp);

        btnStartPing = new QPushButton("Запустить Ping", this);
        btnStartPing->setStyleSheet("background-color: #28a745; color: white; font-weight: bold; padding: 6px;");
        pingLayout->addWidget(btnStartPing);

        txtPingOutput = new QTextEdit(this);
        txtPingOutput->setReadOnly(true);
        txtPingOutput->setStyleSheet("background-color: #1e1e1e; color: #00ff00; font-family: monospace;");
        pingLayout->addWidget(txtPingOutput);

        tabWidget->addTab(macTab, "MAC Changer");
        tabWidget->addTab(pingTab, "Gateway Ping");
        mainLayout->addWidget(tabWidget);

        pingProcess = new QProcess(this);

        connect(comboInterfaces, &QComboBox::currentTextChanged, this, &MacChangerWidget::updateCurrentMac);
        connect(comboInterfaces, &QComboBox::currentTextChanged, this, &MacChangerWidget::autoDetectGateway);
        connect(comboProfiles, &QComboBox::currentTextChanged, this, &MacChangerWidget::updateProfileMac);
        connect(btnReloadConfig, &QPushButton::clicked, this, &MacChangerWidget::reloadConfigAction);
        connect(btnApply, &QPushButton::clicked, this, &MacChangerWidget::applyMacChange);
        connect(btnRestoreDefault, &QPushButton::clicked, this, &MacChangerWidget::setNativeMac);

        connect(spinThirdOctet, &QSpinBox::valueChanged, this, &MacChangerWidget::onOctetChanged);
        connect(btnStartPing, &QPushButton::clicked, this, &MacChangerWidget::togglePing);
        connect(pingProcess, &QProcess::readyReadStandardOutput, this, &MacChangerWidget::readPingOutput);
    }

    /* Загрузка конфига в переменные */
    void loadConfig() {
        settings->beginGroup(sectionName);
        comboProfiles->clear();
        comboProfiles->addItem("[Ввести кастомный MAC]");
        
        for (const QString &key : settings->allKeys()) {
            if (!key.startsWith("HELP_")) {
                comboProfiles->addItem(key);
            }
        }
        settings->endGroup();
    }

    /* Загрузка имеющихся интерфейсов из системы (кроме loopback) */
    void loadSystemInterfaces() {
        QProcess process;
        process.start("ip", QStringList() << "-o" << "link" << "show");
        if (process.waitForFinished()) {
            QString output = QString::fromUtf8(process.readAllStandardOutput());
            QStringList lines = output.split('\n');
            QStringList interfaces;
            for (const QString &line : lines) {
                if (line.isEmpty()) continue;
                QStringList parts = line.split(": ");
                if (parts.size() > 1) { 
                    QString ifaceName = parts.at(1).trimmed();
                    
                    if (ifaceName == "lo") continue; 
                    
                    interfaces.append(ifaceName); 
                }
            }
            comboInterfaces->addItems(interfaces);
        }
        QString defaultNet = settings->value("DEFAULTS/interface", "wlan0").toString();
        int index = comboInterfaces->findText(defaultNet);
        if (index != -1) { comboInterfaces->setCurrentIndex(index); }
    }

    /* Перезагрузка конфигов с диска */
    void reloadConfigAction() {
        if (settings) {
            delete settings;
            settings = nullptr;
        }
        
        initPaths();
        
        loadConfig();
        
        updateProfileMac(comboProfiles->currentText());
        
        QMessageBox::information(this, "Успех", "Конфигурационный файл успешно перечитан с диска!");
    }


    /* Обновляет строчку с текущим маком (смотрим в системе) при выборе в дропбоксе. Показывает энтри для ввода кастомного если выбран пункт в меню. */
    void updateProfileMac(const QString &profileName) {
        if (profileName.isEmpty()) return;

        disconnect(editCustomMac, &QLineEdit::textChanged, this, nullptr);

        if (profileName == "[Ввести кастомный MAC]") {
            labelProfileMac->setText("Ручной ввод...");
            editCustomMac->show();
            
            connect(editCustomMac, &QLineEdit::textChanged, this, [this](const QString &text){
                QRegularExpression macRegex("^([0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2}$");
                bool isValid = macRegex.match(text).hasMatch();
                btnApply->setEnabled(isValid);
                
                if (!isValid && !text.isEmpty()) {
                    editCustomMac->setStyleSheet("border: 1px solid #cc0000; background-color: #ffcccc;");
                }
                else {
                    editCustomMac->setStyleSheet("");
                }
            });
            
            editCustomMac->textChanged(editCustomMac->text());

        }
        else {
            editCustomMac->hide();
            
            QString mac = settings->value(sectionName + "/" + profileName).toString().trimmed();
            labelProfileMac->setText(mac.isEmpty() ? "Не найден" : mac);

            QRegularExpression macRegex("^([0-9A-Fa-f]{2}[:-]){5}([0-9A-Fa-f]{2})$");
            if (!macRegex.match(mac).hasMatch()) {
                labelProfileMac->setStyleSheet("background-color: #ffcccc; color: #cc0000; font-weight: bold; padding: 4px; border: 1px solid #cc0000; border-radius: 3px;");
                btnApply->setEnabled(false);
            } else {
                labelProfileMac->setStyleSheet("background-color: #e0e0e0; color: #000000; font-weight: bold; padding: 4px; border: 1px solid #b0b0b0; border-radius: 3px;");
                btnApply->setEnabled(true);
            }
        }
    }

    /* Обновляет строчку с текущим маком (смотрим в системе) при выборе в дропбоксе */
    void updateCurrentMac(const QString &interface) {
        if (interface.isEmpty()) return;
        QProcess process;
        process.start("ip", QStringList() << "link" << "show" << interface);
        if (process.waitForFinished()) {
            QString output = QString::fromUtf8(process.readAllStandardOutput());
            for (const QString &line : output.split('\n')) {
                if (line.contains("ether")) {
                    QRegularExpression macRegex("([0-9A-Fa-f]{2}[:-]){5}[0-9A-Fa-f]{2}");
                    QRegularExpressionMatch match = macRegex.match(line);
                    if (match.hasMatch()) {
                        labelCurrentMac->setText(match.captured(0));
                    }
                }
            }
        }
        else{
            labelCurrentMac->setText("Не задан / Динамический");
        }
        QProcess infoProcess;
        infoProcess.start("ip", QStringList() << "addr" << "show" << interface);
        if (infoProcess.waitForFinished()) {
            QString rawInfo = QString::fromUtf8(infoProcess.readAllStandardOutput()).trimmed();
            txtInterfaceInfo->setPlainText(rawInfo);
        } else {
            txtInterfaceInfo->setPlainText("Не удалось получить расширенную информацию.");
        }
    }

    /* Смотрит есть ли дефолт для интерфейса и создает дефолтный алиас */
    void setNativeMac() {
        QString currentDev = comboInterfaces->currentText();
        if (currentDev.isEmpty()) return;

        QString nativeMac = settings->value("DEFAULTS/" + currentDev).toString().trimmed();
        if (nativeMac.isEmpty()) {
            QMessageBox::information(this, "Нет данных", 
                QString("В конфиге под секцией [DEFAULTS] не найден мак для <b>%1</b>.").arg(currentDev));
            return;
        }

        int existIndex = comboProfiles->findText("default_" + currentDev);
        if (existIndex != -1) {
            comboProfiles->setCurrentIndex(existIndex);
        } else {
            settings->setValue(sectionName + "/default_" + currentDev, nativeMac);
            settings->sync();
            loadConfig(); 
            comboProfiles->setCurrentText("default_" + currentDev);
        }
    }

    /* Автоматически определяет IP-адрес шлюза (роутера) для выбранного интерфейса и подставляет его в поле пинга */
    void autoDetectGateway(const QString &interface="") {
        QString dev = interface.isEmpty() ? comboInterfaces->currentText() : interface;
        if (dev.isEmpty()) return;
        QProcess process;
        process.start("ip", QStringList() << "route" << "show" << "dev" << dev);
        if (process.waitForFinished()) {
            QString output = QString::fromUtf8(process.readAllStandardOutput());
            for (const QString &line : output.split('\n')) {
                if (line.startsWith("default via")) {
                    QStringList tokens = line.split(" ");
                    if (tokens.size() > 2) {
                        QString gwIp = tokens.at(2);
                        editCustomIp->setText(gwIp);
                        QStringList ipParts = gwIp.split(".");
                        if (ipParts.size() == 4 && ipParts.at(0) == "192" && ipParts.at(1) == "168") {
                            spinThirdOctet->setValue(ipParts.at(2).toInt());
                        }
                        return;
                    }
                }
            }
        }
        editCustomIp->setText("192.168.0.1");
    }

    /* Изменяет в энтри 3 число адреса гейтвея по выбору выше */
    void onOctetChanged(int val) {
        editCustomIp->setText(QString("192.168.%1.1").arg(val));
    }

    /* Запускает или останавливает пинг */
    void togglePing() {
        if (pingProcess->state() == QProcess::Running) {
            pingProcess->kill();
            btnStartPing->setText("Запустить Ping");
            btnStartPing->setStyleSheet("background-color: #28a745; color: white; font-weight: bold; padding: 6px;");
            txtPingOutput->append("\n--- Пинг остановлен ---");
        } else {
            txtPingOutput->clear();
            QString targetIp = editCustomIp->text().trimmed();
            pingProcess->start("ping", QStringList() << targetIp);
            btnStartPing->setText("Остановить Ping");
            btnStartPing->setStyleSheet("background-color: #dc3545; color: white; font-weight: bold; padding: 6px;");
        }
    }

    /* Перенаправляет вывод пинга в виджет */
    void readPingOutput() {
        QByteArray data = pingProcess->readAllStandardOutput();
        txtPingOutput->append(QString::fromUtf8(data).trimmed());
    }

    /* Смена мак адреса */
    void applyMacChange() {
        QString interface = comboInterfaces->currentText();
        QString profile = comboProfiles->currentText();
        if (interface.isEmpty() || profile.isEmpty()) return;

        QString mac;
        if (profile == "[Ввести кастомный MAC]") {
            mac = editCustomMac->text().trimmed().toUpper();
        } else {
            mac = settings->value(sectionName + "/" + profile).toString();
        }

        QMessageBox::StandardButton reply = QMessageBox::question(this, "Подтверждение", 
                                    QString("Изменить MAC-адрес на <b>%1</b> на <b>%2</b>?").arg(interface, mac),
                                    QMessageBox::Yes|QMessageBox::No);
        if (reply == QMessageBox::No) return;

        QString script = QString(
            "ip link set dev %1 down\n"
            "ip link set dev %1 address %2\n"
            "ip link set dev %1 up\n"
        ).arg(interface, mac);

        QProcess process;
        process.start("pkexec", QStringList() << "sh"); 
        process.write(script.toUtf8());
        process.closeWriteChannel();

        if (process.waitForFinished() && process.exitCode() == 0) {
            QMessageBox::information(this, "Успех", "MAC-адрес успешно изменен!");
            updateCurrentMac(interface);
        } else {
            QMessageBox::critical(this, "Ошибка", "Ошибка выполнения. Проверьте права доступа.");
        }
    }
};

/* Запуск программы */
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    QApplication::setQuitOnLastWindowClosed(false); 
    
    QCommandLineParser parser;
    QCommandLineOption trayOption("tray", "Запустить свернутым в трей");
    parser.addOption(trayOption);
    parser.process(app);

    bool startInTray = parser.isSet(trayOption);
    
    bool isTrayAvailable = QSystemTrayIcon::isSystemTrayAvailable();
    
    MacChangerWidget window;
    
    if (startInTray && isTrayAvailable) { // Запуск в трее произойдет только если был указан флаг --tray и трей существует в системе
        window.hide(); 
    } else {
        window.showNormal(); 
    }
    
    return app.exec();
}