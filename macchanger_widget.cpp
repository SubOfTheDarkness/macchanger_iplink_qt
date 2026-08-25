#include "macchanger_widget.h"
#include "ui_macchanger_widget.h"
#include "ping_tab.h"
#include "icon.xpm"
#include <QMessageBox>
#include <QStyle>
#include <QMenu>
#include <QCloseEvent>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include <QCoreApplication>
#include <QRegularExpression>
#include <QPushButton>
#include <QShortcut>
#include <QKeySequence>
#include <QTextEdit>
#include <QDialog>
#include <QVBoxLayout>
#include <QStandardPaths>
#include <QSysInfo>

/* 
 * Конструктор главного окна. Инициализирует разметку UI, устанавливает маску ввода MAC,
 * включает крестики закрытия табов пинга, а также последовательно запускает 
 * инициализацию путей, трея, связей, хоткеев и системных конфигураций.
 * В конце определяет режим отображения (обычный или скрытый в трей).
 */
MacChangerWidget::MacChangerWidget(bool hasTraySupport, bool startInTray, QWidget *parent) 
    : QWidget(parent)
    , ui(new Ui::macchanger_widget)
    , settings(nullptr)
    , trayIcon(nullptr)
    , m_hasTraySupport(hasTraySupport)
{
    sectionName = "MAC_ALIASES"; 
    ui->setupUi(this);

    ui->mac_address_entry->setInputMask("HH:HH:HH:HH:HH:HH;_");

    initPaths();
    
    if (m_hasTraySupport) {
        initTray();
    }

    QPixmap iconPixmap(icon_xpm); 
    this->setWindowIcon(QIcon(iconPixmap)); 
    
    initConnections();
    initShortcuts(); 
    loadConfig();
    loadSystemInterfaces();
    
    updateCurrentMac(ui->mac_iface_dropbox->currentText());
    updateProfileMac(ui->mac_alias_dropbox->currentText());
    
    ui->subtabs_ping->setTabsClosable(true);
    connect(ui->subtabs_ping, &QTabWidget::tabCloseRequested, this, &MacChangerWidget::closePingTab);

    createNewPingTab();

    statusBarLabel = new QLabel(this);
    statusBarLabel->setText(QString(" Version: %1 | OS: Linux").arg(QCoreApplication::applicationVersion()));
    
    statusBarLabel->setStyleSheet(
        "QLabel {"
        "    font-size: 11px;"
        "    padding: 3px 5px;"
        "    border-top: 1px solid rgba(128, 128, 128, 0.3);"
        "}"
    );
    
    ui->verticalLayout->addWidget(statusBarLabel);

    ui->sett_autostart_switch->blockSignals(true);
    ui->sett_autostart_switch->setChecked(isAutoStartEnabled());
    ui->sett_autostart_switch->blockSignals(false);

    setWindowTitle(QString("%1 Toolkit - v%2").arg(windowTitle(), QCoreApplication::applicationVersion()));

    if (startInTray) {
        this->hide();
    } else {
        this->showNormal();
    }
}

/* 
 * Деструктор главного окна. Освобождает оперативную память, 
 * занятую автоматически сгенерированным классом разметки UI.
 */
MacChangerWidget::~MacChangerWidget() {
    delete ui;
}

/* 
 * Перехватывает событие закрытия окна (нажатие на крестик). 
 * Если операционная система поддерживает трей и иконка активна, окно скрывается, 
 * а само приложение остается работать в фоне. В противном случае программа закрывается.
 */
void MacChangerWidget::closeEvent(QCloseEvent *event) {
    if (m_hasTraySupport && trayIcon && trayIcon->isVisible()) {
        this->hide();
        event->ignore();
    } else {
        event->accept(); 
    }
}

/* 
 * Инициализирует пути к конфигурационным файлам. Создает скрытую папку проекта 
 * в домашней директории пользователя (~/.config/macchanger/) и пустой файл настроек, 
 * если они отсутствуют. Затем выполняет слияние с дефолтными настройками из ресурсов.
 */
void MacChangerWidget::initPaths() {
    externalConfigPath = QDir::homePath() + "/.config/macchanger/address_aliases.ini";
    
    QFileInfo fileInfo(externalConfigPath);
    if (!fileInfo.exists()) {
        QDir().mkpath(fileInfo.absolutePath());
        QFile file(externalConfigPath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << "; External configuration file. Overrides defaults.\n";
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

/* 
 * Конструирует системный значок утилиты в трее (области уведомлений) ОС.
 * Задает иконку, всплывающую подсказку, собирает контекстное меню (Deploy, Restart, Exit) 
 * и настраивает разворачивание/сворачивание главного окна по клику на иконку.
 */
void MacChangerWidget::initTray() {
    trayIcon = new QSystemTrayIcon(this);

    QPixmap iconPixmap(icon_xpm); 
    trayIcon->setIcon(QIcon(iconPixmap)); 

    trayIcon->setToolTip("MAC Changer Tool");

    QMenu *trayMenu = new QMenu(this);
    QAction *actShow = trayMenu->addAction("Deploy");
    QAction *actRestart = trayMenu->addAction("Restart App");
    QAction *actExit = trayMenu->addAction("Exit");

    trayIcon->setContextMenu(trayMenu);
    trayIcon->show();

    connect(actShow, &QAction::triggered, this, &QWidget::showNormal);
    connect(actExit, &QAction::triggered, []() { qApp->quit(); });
    connect(actRestart, &QAction::triggered, this, &MacChangerWidget::handleCtrlShiftR);

    connect(trayIcon, &QSystemTrayIcon::activated, [this](QSystemTrayIcon::ActivationReason reason){
        if (reason == QSystemTrayIcon::Trigger) {
            if (this->isVisible()) this->hide();
            else this->showNormal();
        }
    });
}

/* 
 * Регистрирует сигналы и слоты для всех интерактивных элементов формы (дропбоксы, кнопки).
 * Устанавливает регулярное выражение на поле ввода MAC-адреса и настраивает 
 * динамическое изменение цвета его фона, если адрес введен некорректно или не полностью.
 */
void MacChangerWidget::initConnections() {
    connect(ui->mac_iface_dropbox, &QComboBox::currentTextChanged, this, &MacChangerWidget::updateCurrentMac);
    connect(ui->mac_alias_dropbox, &QComboBox::currentTextChanged, this, &MacChangerWidget::updateProfileMac);
    
    connect(ui->mac_cfg_reload_btn, &QPushButton::clicked, this, &MacChangerWidget::reloadConfigAction);
    connect(ui->mac_apply_btn, &QPushButton::clicked, this, &MacChangerWidget::applyMacChange);
    
    connect(ui->mac_default_addr_btn, &QPushButton::clicked, this, &MacChangerWidget::setNativeMac);
    connect(ui->mac_iface_info_btn, &QPushButton::clicked, this, &MacChangerWidget::showInterfaceInfo);

    connect(ui->ping_add_tab_btn, &QPushButton::clicked, this, &MacChangerWidget::createNewPingTab);

    connect(ui->sett_autostart_switch, &QCheckBox::toggled, this, &MacChangerWidget::onAutostartToggled);

    connect(ui->sett_about_btn, &QPushButton::clicked, this, &MacChangerWidget::showAboutDialog);

    connect(ui->mac_address_entry, &QLineEdit::textChanged, this, [this](const QString &text) {
        bool isValid = ui->mac_address_entry->hasAcceptableInput();
        ui->mac_apply_btn->setEnabled(isValid);
        
        QString rawText = text;
        rawText.remove(':').remove('_');
        
        bool hasError = !isValid && !rawText.trimmed().isEmpty();
        
        if (ui->mac_address_entry->property("error").toBool() != hasError) {
            ui->mac_address_entry->setProperty("error", hasError);
            ui->mac_address_entry->style()->unpolish(ui->mac_address_entry);
            ui->mac_address_entry->style()->polish(ui->mac_address_entry);
        }
    });

}

/* 
 * Считывает все сохраненные профили и алиасы MAC-адресов из INI-файла настроек 
 * и заполняет ими выпадающий список на форме, предварительно добавив пункт кастомного ввода.
 */
void MacChangerWidget::loadConfig() {
    settings->beginGroup(sectionName);
    ui->mac_alias_dropbox->clear();
    ui->mac_alias_dropbox->addItem("[Enter Custom]");
    
    for (const QString &key : settings->allKeys()) {
        if (!key.startsWith("HELP_")) {
            ui->mac_alias_dropbox->addItem(key);
        }
    }
    settings->endGroup();
}

/* 
 * Инициализирует глобальные горячие клавиши приложения и связывает их с соответствующими слотами.
 */
void MacChangerWidget::initShortcuts() {
    // Ctrl+Q - Полное закрытие программы
    QShortcut *shortcutCloseAll = new QShortcut(QKeySequence("Ctrl+Q"), this);
    connect(shortcutCloseAll, &QShortcut::activated, []() { qApp->quit(); });

    // Ctrl+W - Закрыть окно (свернуться в трей)
    QShortcut *shortcutMinimize = new QShortcut(QKeySequence("Ctrl+W"), this);
    connect(shortcutMinimize, &QShortcut::activated, this, &QWidget::hide);

    // Ctrl+T - Новая вкладка пинга
    QShortcut *shortcutNewTab = new QShortcut(QKeySequence("Ctrl+T"), this);
    connect(shortcutNewTab, &QShortcut::activated, this, &MacChangerWidget::handleCtrlT);

    // Ctrl+R - Обновить конфиг
    QShortcut *shortcutReloadConfig = new QShortcut(QKeySequence("Ctrl+R"), this);
    connect(shortcutReloadConfig, &QShortcut::activated, this, &MacChangerWidget::handleCtrlR);

    // Ctrl+Shift+R - Перезапустить окно приложения
    QShortcut *shortcutRestartApp = new QShortcut(QKeySequence("Ctrl+Shift+R"), this);
    connect(shortcutRestartApp, &QShortcut::activated, this, &MacChangerWidget::handleCtrlShiftR);

    // Ctrl+Enter - Запуск пинга или применение MAC
    QShortcut *shortcutConfirm = new QShortcut(QKeySequence("Ctrl+Return"), this);
    connect(shortcutConfirm, &QShortcut::activated, this, &MacChangerWidget::handleCtrlEnter);

    // Ctrl+Shift+W - Закрыть текущую вкладку пинга
    QShortcut *shortcutCloseTab = new QShortcut(QKeySequence("Ctrl+Shift+W"), this);
    connect(shortcutCloseTab, &QShortcut::activated, this, &MacChangerWidget::handleCtrlShiftW);
}

/* 
 * handleCtrlT: Слот для Ctrl+T. Создает новую вкладку сетевого мониторинга (пинга), 
 * если пользователь находится в данный момент на соответствующей вкладке приложения.
 */
void MacChangerWidget::handleCtrlT() {
    if (ui->main_tabs->currentIndex() == 1) {
        createNewPingTab();
    }
}

/* 
 * handleCtrlR: Слот для Ctrl+R. Перезагружает конфигурационный файл профилей с диска, 
 * если активна главная вкладка макченджера.
 */
void MacChangerWidget::handleCtrlR() {
    if (ui->main_tabs->currentIndex() == 0) {
        reloadConfigAction();
    }
}

/* 
 * handleCtrlShiftR: Слот для Ctrl+Shift+R. Перезапускает приложение: создает независимый 
 * дочерний процесс текущего бинарника и завершает работу текущей сессии утилиты.
 */
void MacChangerWidget::handleCtrlShiftR() {
    QProcess::startDetached(QCoreApplication::applicationFilePath(), QCoreApplication::arguments());
    qApp->quit();
}

/* 
 * handleCtrlShiftW: Слот для Ctrl+Shift+W. Находит индекс текущей активной вкладки пинга 
 * и, если она существует, отправляет её на процедуру закрытия.
 */
void MacChangerWidget::handleCtrlShiftW() {
    if (ui->main_tabs->currentIndex() == 1) {
        int currentSubTabIndex = ui->subtabs_ping->currentIndex();
        
        if (currentSubTabIndex != -1) {
            closePingTab(currentSubTabIndex);
        }
    }
}

/* 
 * handleCtrlEnter: Слот для Ctrl+Return. На вкладке макченджера имитирует нажатие кнопки Apply. 
 * На вкладке пинга находит активный монитор и программно нажимает на его кнопку Start/Stop.
 */
void MacChangerWidget::handleCtrlEnter() {
    int currentTab = ui->main_tabs->currentIndex();
    
    if (currentTab == 0) {
        if (ui->mac_apply_btn->isEnabled()) {
            applyMacChange();
        }
    } 
    else if (currentTab == 1) {
        PingTab *activePingTab = qobject_cast<PingTab*>(ui->subtabs_ping->currentWidget());
        if (activePingTab) {
            QPushButton *toggleBtn = activePingTab->findChild<QPushButton*>("ping_toggle_btn");
            if (toggleBtn) {
                toggleBtn->click();
            }
        }
    }
}

/* 
 * Вызывает системную утилиту 'ip -o link show' для сканирования сетевых интерфейсов Linux. 
 * Парсит текстовый ответ, отсекает локальную петлю/лупбак (loopback, lo) и наполняет выпадающий список dropbox. 
 * В конце выставляет интерфейс по умолчанию из файла конфигурации.
 */
void MacChangerWidget::loadSystemInterfaces() {
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
        ui->mac_iface_dropbox->addItems(interfaces);
    }
    QString defaultNet = settings->value("DEFAULTS/interface", "wlan0").toString();
    int index = ui->mac_iface_dropbox->findText(defaultNet);
    if (index != -1) { ui->mac_iface_dropbox->setCurrentIndex(index); }
}

/* 
 * Сбрасывает текущий объект настроек в ОЗУ, заново перечитывает файл конфигурации с диска, 
 * обновляет элементы GUI и выводит информационное окно об успешном завершении операции.
 */
void MacChangerWidget::reloadConfigAction() {
    if (settings) {
        delete settings;
        settings = nullptr;
    }
    initPaths();
    loadConfig();
    updateProfileMac(ui->mac_alias_dropbox->currentText());
    QMessageBox::information(this, "Success", "The configuration file has been successfully re‑read from the disk.");
}

/* 
 * Считывает имя выбранной сетевой карты и запускает процесс 'ip addr show'. 
 * Программно создает модальное диалоговое окно QDialog со стилизованным текстовым полем QTextEdit 
 * в режиме терминала и выводит туда подробную системную информацию об интерфейсе.
 */
void MacChangerWidget::showInterfaceInfo() {
    QString interface = ui->mac_iface_dropbox->currentText();
    if (interface.isEmpty()) return;

    QProcess infoProcess;
    infoProcess.start("ip", QStringList() << "addr" << "show" << interface);
    
    QString rawInfo = "Extended information could not be obtained.";
    if (infoProcess.waitForFinished()) {
        rawInfo = QString::fromUtf8(infoProcess.readAllStandardOutput()).trimmed();
    }

    QDialog *infoDialog = new QDialog(this);
    infoDialog->setWindowTitle("Interface Info: " + interface);
    infoDialog->setMinimumSize(450, 250);

    auto *layout = new QVBoxLayout(infoDialog);
    QTextEdit *txtInfo = new QTextEdit(infoDialog);
    txtInfo->setReadOnly(true);
    txtInfo->setStyleSheet("background-color: #2d3436; color: #00cec9; font-family: monospace; font-size: 11px;");
    txtInfo->setPlainText(rawInfo);

    layout->addWidget(txtInfo);
    infoDialog->setLayout(layout);
    infoDialog->setAttribute(Qt::WA_DeleteOnClose); 
    infoDialog->exec(); 
}

/* 
 * Срабатывает при смене профиля в выпадающем списке. Если выбран ручной ввод, 
 * показывает текстовое поле QLineEdit. Если выбран готовый профиль, поле ввода скрывается, 
 * а в лейбл выводится MAC-адрес алиаса. В случае битого адреса в конфиге лейбл подсвечивается красным.
 */
void MacChangerWidget::updateProfileMac(const QString &profileName) {
    if (profileName.isEmpty()) return;

    if (profileName == "[Enter Custom]") {
        ui->mac_alias_address_lbl->hide();
        ui->info_mac_alias_address_lbl->setText("Enter custom address:");
        ui->mac_address_entry->show(); 
        ui->mac_apply_btn->setEnabled(ui->mac_address_entry->hasAcceptableInput());
    } 
    else {
        ui->mac_address_entry->hide(); 
        ui->mac_alias_address_lbl->show();
        ui->info_mac_alias_address_lbl->setText("Alias MAC:");
        ui->mac_address_entry->setStyleSheet(""); 
        
        QString mac = settings->value(sectionName + "/" + profileName).toString().trimmed();
        ui->mac_alias_address_lbl->setText(mac.isEmpty() ? "Not Found" : mac);

        bool isConfigMacValid = macRegex.match(mac).hasMatch();
        
        ui->mac_apply_btn->setEnabled(isConfigMacValid);
        ui->mac_alias_address_lbl->setProperty("error", !isConfigMacValid);
        
        ui->mac_alias_address_lbl->style()->unpolish(ui->mac_alias_address_lbl);
        ui->mac_alias_address_lbl->style()->polish(ui->mac_alias_address_lbl);
    }
}

/* 
 * Запускает команду 'ip link show' для конкретного сетевого интерфейса ОС Linux. 
 * С помощью регулярного выражения находит строку 'ether' и вычленяет из нее 
 * текущий установленный физический MAC-адрес оборудования для вывода на экран.
 */
void MacChangerWidget::updateCurrentMac(const QString &interface) {
    if (interface.isEmpty()) return;
    
    QProcess process;
    process.start("ip", QStringList() << "link" << "show" << interface);
    
    if (process.waitForFinished()) {
        QString output = QString::fromUtf8(process.readAllStandardOutput());
        bool found = false;
        
        for (const QString &line : output.split('\n')) {
            if (line.contains("ether")) {
                QRegularExpressionMatch match = macRegex.match(line);
                if (match.hasMatch()) {
                    ui->mac_current_lbl->setText(match.captured(0));
                    found = true;
                    break;
                }
            }
        }
        if (!found) ui->mac_current_lbl->setText("Not specified / Dynamic");
    } else {
        ui->mac_current_lbl->setText("Interface reading error");
    }
}


/* 
 * Считывает заводской (родной) MAC-адрес интерфейса из эталонной секции [DEFAULTS] INI-файла. 
 * Если запись найдена, переключает комбобокс профилей на соответствующий пункт, 
 * избавляя от необходимости прописывать его вручную.
 */
void MacChangerWidget::setNativeMac() {
    QString currentDev = ui->mac_iface_dropbox->currentText(); 
    if (currentDev.isEmpty()) return;

    QString nativeMac = settings->value("DEFAULTS/" + currentDev).toString().trimmed();
    if (nativeMac.isEmpty()) {
        QMessageBox::information(this, "No data", 
            QString("In the config, under the [DEFAULTS] section, no address for <b>%1</b> was found.").arg(currentDev));
        return;
    }

    int existIndex = ui->mac_alias_dropbox->findText("default_" + currentDev); 
    if (existIndex != -1) {
        ui->mac_alias_dropbox->setCurrentIndex(existIndex);
    } else {
        settings->setValue(sectionName + "/default_" + currentDev, nativeMac);
        settings->sync();
        loadConfig(); 
        ui->mac_alias_dropbox->setCurrentText("default_" + currentDev);
    }
}

/* 
 * Запрашивает подтверждение операции у пользователя. Формирует последовательный bash-скрипт 
 * смены адреса (ip link set down -> set address -> set up) и отправляет его на выполнение 
 * в системный интерпретатор с повышением привилегий через утилиту pkexec.
 */
void MacChangerWidget::applyMacChange() {
    QString interface = ui->mac_iface_dropbox->currentText();
    QString profile = ui->mac_alias_dropbox->currentText();
    if (interface.isEmpty() || profile.isEmpty()) return;

    QString mac;
    if (profile == "[Enter Custom]") {
        mac = ui->mac_address_entry->text().trimmed().toUpper();
    } else {
        mac = settings->value(sectionName + "/" + profile).toString();
    }

    QMessageBox::StandardButton reply = QMessageBox::question(this, "Confirmation", 
                                QString("Change the MAC address to <b>%1</b> on interface <b>%2</b>?").arg(mac, interface),
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
        QMessageBox::information(this, "Success", "MAC-address successfully changed!");
        updateCurrentMac(interface);
    } else {
        QMessageBox::critical(this, "Error", "Execution error. Check the access rights.");
    }
}

/* 
 * Динамически создает новый изолированный экземпляр виджета PingTab в куче, 
 * рассчитывает для него красивый заголовок по типу '№1' и добавляет в контейнер. 
 * Также подключает лямбда-перехватчик для динамического добавления статуса [Active] к заголовку.
 */
void MacChangerWidget::createNewPingTab() {
    PingTab *newPingTab = new PingTab(this);
    
    int currentTabCount = ui->subtabs_ping->count();
    QString tabTitle = QString("№%1").arg(currentTabCount + 1);

    int newIndex = ui->subtabs_ping->addTab(newPingTab, tabTitle);
    ui->subtabs_ping->setCurrentIndex(newIndex);

    connect(newPingTab, &PingTab::statusChanged, this, [this, newPingTab](bool isRunning) {
        int idx = ui->subtabs_ping->indexOf(newPingTab);
        if (idx != -1) {
            QString newTitle = QString("№%1").arg(idx + 1);
            if (isRunning) {
                ui->subtabs_ping->setTabText(idx, newTitle + " [Active]");
            } else {
                ui->subtabs_ping->setTabText(idx, newTitle);
            }
        }
    });
}

/* 
 * Закрывает вкладку сетевого мониторинга по её индексу. Включает защиту 
 * (не позволяет удалить единственный оставшийся таб). После удаления запускает цикл 
 * перенумерации оставшихся вкладок, полностью исключая дыры в порядке номеров.
 */
void MacChangerWidget::closePingTab(int index) {
    QWidget *tabPage = ui->subtabs_ping->widget(index);
    if (tabPage) {
        if (ui->subtabs_ping->count() <= 1) {
            return; 
        }

        ui->subtabs_ping->removeTab(index);
        tabPage->deleteLater(); 

        for (int i = 0; i < ui->subtabs_ping->count(); ++i) {
            QString currentText = ui->subtabs_ping->tabText(i);
            QString statusSuffix = currentText.contains("[Active]") ? " [Active]" : "";
            ui->subtabs_ping->setTabText(i, QString("№%1%2").arg(i + 1).arg(statusSuffix));
        }
    }
}

/* Слот для переключения автозапуска */
void MacChangerWidget::onAutostartToggled(bool checked) {
    setAutoStart(checked);
}

/* Переключение автозапуска с проверкой на уже существующий экземпляр в автозапуске(в том же файле, другие не учитываются) и попапом на ошибку */
void MacChangerWidget::setAutoStart(bool enable) {
    QString autostartDir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/autostart";
    QDir().mkpath(autostartDir);

    QString appName = QCoreApplication::applicationName();
    
    QString filePath = autostartDir + "/" + appName.toLower() + ".desktop";
    QString currentExecPath = QString("\"%1\" --tray").arg(QCoreApplication::applicationFilePath());

    if (enable) {
        QSettings autoSettings(filePath, QSettings::IniFormat);
        autoSettings.beginGroup("Desktop Entry");
        autoSettings.setValue("Type", "Application");
        autoSettings.setValue("Name", "MacChanger ToolKit");
        autoSettings.setValue("Comment", "Fast MAC address changer and network ping toolkit");
        autoSettings.setValue("Exec", currentExecPath);
        autoSettings.setValue("Icon", "macchanger-toolkit");
        autoSettings.setValue("Terminal", "false");
        autoSettings.setValue("Hidden", "false");
        autoSettings.setValue("NoDisplay", "false");
        autoSettings.setValue("X-GNOME-Autostart-enabled", "true");
        autoSettings.endGroup();

        autoSettings.sync();
        
        if (autoSettings.status() != QSettings::NoError || !QFile::exists(filePath)) {
            QMessageBox::critical(this, "Autostart Error", 
                "Failed to write the autostart configuration file. Please check folder permissions.");
            
            ui->sett_autostart_switch->blockSignals(true);
            ui->sett_autostart_switch->setChecked(false);
            ui->sett_autostart_switch->blockSignals(false);
        }
    } else {
        if (QFile::exists(filePath)) {
            QSettings autoSettings(filePath, QSettings::IniFormat);
            QString savedExec = autoSettings.value("Desktop Entry/Exec").toString();

            if (!savedExec.isEmpty() && savedExec != currentExecPath) {
                QMessageBox::StandardButton reply = QMessageBox::warning(this, "Alternative App Detected",
                    "The existing autostart entry points to a different instance or location of this application.\n\n"
                    "Are you sure you want to delete it anyway?",
                    QMessageBox::Yes | QMessageBox::No);

                if (reply == QMessageBox::No) {
                    ui->sett_autostart_switch->blockSignals(true);
                    ui->sett_autostart_switch->setChecked(true);
                    ui->sett_autostart_switch->blockSignals(false);
                    return;
                }
            }
        }
        
        QFile::remove(filePath);
    }
}

/* Переключение состояние свитча автозапуска в зависимости от наличия файла */
bool MacChangerWidget::isAutoStartEnabled() {
    QString appName = QCoreApplication::applicationName();
    
    QString filePath = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/autostart/" + appName.toLower() + ".desktop";
    
    return QFile::exists(filePath);
}

/* Диалог About(лицензия) */
void MacChangerWidget::showAboutDialog() {
    QDialog *aboutDialog = new QDialog(this);
    aboutDialog->setWindowTitle("system_info --about");
    aboutDialog->setMinimumSize(520, 380);

    QVBoxLayout *layout = new QVBoxLayout(aboutDialog);
    layout->setContentsMargins(10, 10, 10, 10);

    QTextEdit *txtAbout = new QTextEdit(aboutDialog);
    txtAbout->setReadOnly(true);
    
    txtAbout->setStyleSheet(
        "QTextEdit {"
        "    background-color: #0c0c0c;"
        "    border: 1px solid #222222;"
        "    font-family: 'Source Code Pro', 'Fira Code', 'Courier New', monospace;"
        "    font-size: 12px;"
        "}"
    );

    QString systemUser = qgetenv("USER");
    if (systemUser.isEmpty()) systemUser = "user";
    
    QString systemHost = QSysInfo::machineHostName();
    if (systemHost.isEmpty()) systemHost = "linux";

    QString globalVersion = QCoreApplication::applicationVersion();

    QString paleRed   = "#ff7675";
    QString white     = "#ffffff";
    QString brightRed = "#ff003c";
    QString softYellow= "#f1c40f";

    QString promptTop = QString("<span style='color: %1;'>╭─</span>"
                                "<span style='color: %2;'>%3</span>"
                                "<span style='color: %4;'>@</span>"
                                "<span style='color: %2;'>%5</span> "
                                "<span style='color: %4;'>in</span> "
                                "<span style='color: %4;'>~</span> "
                                "<span style='color: %4;'>took</span> "
                                "<span style='color: %6;'>0s</span>")
                        .arg(paleRed, brightRed, systemUser, white, systemHost, softYellow);

    QString promptBottom = QString("<span style='color: %1;'>╰─λ</span>").arg(paleRed);

    QString htmlContent = QString(
        "%1<br>"
        "%2 ./macchanger --version<br><br>"
        "--------------------------------------------------<br>"
        "<span style='color: #00FF66;'>▶ APPLICATION:</span> MacChanger ToolKit<br>"
        "<span style='color: #00FF66;'>▶ VERSION:    </span> v%3<br>"
        "<span style='color: #00FF66;'>▶ DEVELOPER:  </span> SubOfTheDarkness<br>"
        "<span style='color: #00FF66;'>▶ COPYRIGHT:  </span> Copyright &copy; 2026 Free Software Foundation, Inc.<br>"
        "--------------------------------------------------<br><br>"
        "<span style='color: #E6DB74;'>[LICENSE NOTICE]</span><br>"
        "<span style='color: #888888; font-size: 11px;'>"
        "This program is free software: you can redistribute it and/or modify it "
        "under the terms of the GNU General Public License as published by the Free Software "
        "Foundation, either version 3 of the License, or (at your option) any later version.<br><br>"
        "This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; "
        "without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. "
        "See the GNU General Public License for more details.</span><br><br>"
        "%1<br>"
        "%2 <span style='color: #ffffff; background-color: #ffffff;'>&nbsp;</span>"
    ).arg(promptTop, promptBottom, globalVersion);

    txtAbout->setHtml(htmlContent);
    layout->addWidget(txtAbout);

    QPushButton *btnClose = new QPushButton("exit", aboutDialog);
    btnClose->setStyleSheet(
        "QPushButton {"
        "    background-color: #1e1e1e;"
        "    color: #ff7675;"
        "    border: 1px solid #333333;"
        "    font-family: monospace;"
        "    padding: 5px 15px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #2a2a2a;"
        "    border-color: #ff003c;"
        "    color: #ffffff;"
        "}"
    );
    connect(btnClose, &QPushButton::clicked, aboutDialog, &QDialog::accept);
    
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
    btnLayout->addWidget(btnClose);
    
    layout->addLayout(btnLayout);

    aboutDialog->setAttribute(Qt::WA_DeleteOnClose);
    aboutDialog->exec();
}