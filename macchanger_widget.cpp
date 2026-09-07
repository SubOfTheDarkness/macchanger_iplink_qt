#include "macchanger_widget.h"
#include "ui_macchanger_widget.h"
#include "terminal_about_dialog.h"
#include "terminal_help_dialog.h"
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
#include <QRandomGenerator>
#include <QInputDialog>
#include <QFileDialog>
#include <qcoreapplication.h>

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
    statusBarLabel->setText(QString(" Version: %1").arg(QCoreApplication::applicationVersion()));
    
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

    setWindowTitle(QString("%1 Toolkit").arg(QCoreApplication::applicationName()));

    m_isCentralNotifyLocked = false;

    m_centralNotifyTimer = new QTimer(this);
    m_centralNotifyTimer->setSingleShot(true);
    connect(m_centralNotifyTimer, &QTimer::timeout, this, &MacChangerWidget::sendCentralizedNotification);

    m_centralSpamLockTimer = new QTimer(this);
    m_centralSpamLockTimer->setSingleShot(true);
    connect(m_centralSpamLockTimer, &QTimer::timeout, this, &MacChangerWidget::unlockNotificationSpamProtection);

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
    connect(ui->mac_reload_ifaces_btn, &QPushButton::clicked, this, &MacChangerWidget::loadSystemInterfaces);
    connect(ui->mac_alias_dropbox, &QComboBox::currentTextChanged, this, &MacChangerWidget::updateProfileMac);
    
    connect(ui->cfg_reload_btn, &QPushButton::clicked, this, &MacChangerWidget::reloadConfigAction);
    connect(ui->cfg_open_btn, &QPushButton::clicked, this, &MacChangerWidget::openConfigAction);
    connect(ui->cfg_export_btn, &QPushButton::clicked, this, &MacChangerWidget::exportConfigAction);
    connect(ui->cfg_import_btn, &QPushButton::clicked, this, &MacChangerWidget::importConfigAction);

    connect(ui->mac_apply_btn, &QPushButton::clicked, this, &MacChangerWidget::applyMacChange);
    
    connect(ui->mac_default_addr_btn, &QPushButton::clicked, this, &MacChangerWidget::setNativeMac);
    connect(ui->mac_iface_info_btn, &QPushButton::clicked, this, &MacChangerWidget::showInterfaceInfo);

    connect(ui->mac_random_btn, &QPushButton::clicked, this, &MacChangerWidget::handleRandomMac);
    connect(ui->mac_save_btn, &QPushButton::clicked, this, &MacChangerWidget::handleSaveProfile);

    connect(ui->ping_add_tab_btn, &QPushButton::clicked, this, &MacChangerWidget::createNewPingTab);

    connect(ui->sett_autostart_switch, &QCheckBox::toggled, this, &MacChangerWidget::onAutostartToggled);

    connect(ui->sett_about_btn, &QPushButton::clicked, this, &MacChangerWidget::showAboutDialog);
    connect(ui->sett_help_btn, &QPushButton::clicked, this, &MacChangerWidget::showHelpDialog);

    connect(ui->scan_start_btn, &QPushButton::clicked, this, &MacChangerWidget::startNetworkScan);
    connect(ui->scan_save_btn, &QPushButton::clicked, this, &MacChangerWidget::saveSelectedAlias);
    connect(ui->scan_macchage_btn, &QPushButton::clicked, this, &MacChangerWidget::changeMacFromScanner);

    ui->scan_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->scan_table->setSelectionMode(QAbstractItemView::SingleSelection);

    ui->scan_progress->setVisible(false);

    connect(ui->mac_address_entry, &QLineEdit::textChanged, this, [this](const QString &text) {
        bool isValid = ui->mac_address_entry->hasAcceptableInput();
        ui->mac_apply_btn->setEnabled(isValid);

        ui->mac_save_btn->setEnabled(isValid);
        
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
    QString currentSelected = ui->mac_iface_dropbox->currentText();
    ui->mac_iface_dropbox->blockSignals(true);
    ui->mac_iface_dropbox->clear();

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

    ui->mac_iface_dropbox->blockSignals(false);

    int index = -1;
    if (!currentSelected.isEmpty()) {
        index = ui->mac_iface_dropbox->findText(currentSelected);
    }

    if (index == -1) {
        QString defaultNet = settings->value("DEFAULTS/interface", "wlan0").toString();
        index = ui->mac_iface_dropbox->findText(defaultNet);
    }

    if (index != -1) {
        ui->mac_iface_dropbox->setCurrentIndex(index);
    }

    updateCurrentMac(ui->mac_iface_dropbox->currentText());

    ui->mac_status_lbl->setText("Status: Network interfaces updated.");
    ui->mac_status_lbl->setProperty("state", "default");
    ui->mac_status_lbl->style()->unpolish(ui->mac_status_lbl);
    ui->mac_status_lbl->style()->polish(ui->mac_status_lbl);
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

void MacChangerWidget::openConfigAction() {
    bool success = QProcess::startDetached("xdg-open", QStringList() << externalConfigPath);
}

void MacChangerWidget::exportConfigAction() {
    QString savePath = QFileDialog::getSaveFileName(this, 
        "Export Configuration Backup", 
        QDir::homePath() + "/macchanger_backup.ini", 
        "Configuration Files (*.ini)");

    if (savePath.isEmpty()) return;

    if (QFile::exists(savePath)) {
        QFile::remove(savePath);
    }

    if (QFile::copy(externalConfigPath, savePath)) {
        QMessageBox::information(this, "Export Success", "Your configuration backup has been saved successfully.");
    } else {
        QMessageBox::critical(this, "Export Error", "Failed to export configuration. Access denied or write error.");
    }
}

void MacChangerWidget::importConfigAction() {
    QString importPath = QFileDialog::getOpenFileName(this, 
        "Import Configuration Backup", 
        QDir::homePath(), 
        "Configuration Files (*.ini)");

    if (importPath.isEmpty()) return;

    QSettings backupSettings(importPath, QSettings::IniFormat);
    
    QMap<QString, QString> currentMap;
    QMap<QString, QString> backupMap;

    settings->beginGroup(sectionName);
    for (const QString &key : settings->allKeys()) {
        if (!key.startsWith("HELP_")) {
            currentMap[key] = settings->value(key).toString().trimmed().toUpper();
        }
    }
    settings->endGroup();

    backupSettings.beginGroup(sectionName);
    for (const QString &key : backupSettings.allKeys()) {
        if (!key.startsWith("HELP_")) {
            backupMap[key] = backupSettings.value(key).toString().trimmed().toUpper();
        }
    }
    backupSettings.endGroup();

    struct DiffRecord {
        QString alias;
        QString currentMac;
        QString backupMac;
        QString status;
    };
    QVector<DiffRecord> diffList;

    for (auto it = backupMap.constBegin(); it != backupMap.constEnd(); ++it) {
        DiffRecord rec;
        rec.alias = it.key();
        rec.backupMac = it.value();

        if (!currentMap.contains(it.key())) {
            rec.currentMac = "<Not Found>";
            rec.status = "Added (New)";
            diffList.append(rec);
        } else if (currentMap[it.key()] != it.value()) {
            rec.currentMac = currentMap[it.key()];
            rec.status = "Modified (Conflict)";
            diffList.append(rec);
        }
    }

    for (auto it = currentMap.constBegin(); it != currentMap.constEnd(); ++it) {
        if (!backupMap.contains(it.key())) {
            DiffRecord rec;
            rec.alias = it.key();
            rec.currentMac = it.value();
            rec.backupMac = "<Removed>";
            rec.status = "Removed (Missing)";
            diffList.append(rec);
        }
    }

    if (diffList.isEmpty()) {
        QMessageBox::information(this, "Import Info", "The backup file configuration is completely identical to your current settings.");
        return;
    }

    QDialog *diffDialog = new QDialog(this);
    diffDialog->setWindowTitle("Configuration Conflict Analysis (Diff)");
    diffDialog->setMinimumSize(600, 350);

    QVBoxLayout *mainLayout = new QVBoxLayout(diffDialog);

    QLabel *descLabel = new QLabel("<b>The following differences were detected between your configuration and the backup:</b>", diffDialog);
    mainLayout->addWidget(descLabel);

    QTableWidget *diffTable = new QTableWidget(diffDialog);
    diffTable->setColumnCount(4);
    diffTable->setHorizontalHeaderLabels(QStringList() << "Alias Name" << "Current System MAC" << "Importing Backup MAC" << "Change Status");
    diffTable->setRowCount(diffList.size());
    diffTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    diffTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    for (int i = 0; i < diffList.size(); ++i) {
        diffTable->setItem(i, 0, new QTableWidgetItem(diffList[i].alias));
        diffTable->setItem(i, 1, new QTableWidgetItem(diffList[i].currentMac));
        diffTable->setItem(i, 2, new QTableWidgetItem(diffList[i].backupMac));
        
        QTableWidgetItem *statusItem = new QTableWidgetItem(diffList[i].status);
        if (diffList[i].status.startsWith("Added")) statusItem->setForeground(QBrush(QColor("#28a745")));
        else if (diffList[i].status.startsWith("Modified")) statusItem->setForeground(QBrush(QColor("#00C3FF")));
        else if (diffList[i].status.startsWith("Removed")) statusItem->setForeground(QBrush(QColor("#ff4f4f")));
        
        diffTable->setItem(i, 3, statusItem);
    }
    diffTable->resizeColumnsToContents();
    mainLayout->addWidget(diffTable);

    QLabel *questionLabel = new QLabel("<b>Choose your deployment strategy:</b><br>"
                                       "• <b>Merge:</b> Combine configurations, keeping your local entries and updating conflicts.<br>"
                                       "• <b>Replace:</b> Completely wipe your current config and replace it with the backup.", diffDialog);
    mainLayout->addWidget(questionLabel);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *btnMerge = new QPushButton("Merge Configs", diffDialog);
    QPushButton *btnReplace = new QPushButton("Replace Entirely", diffDialog);
    QPushButton *btnCancel = new QPushButton("Cancel", diffDialog);

    btnLayout->addWidget(btnMerge);
    btnLayout->addWidget(btnReplace);
    btnLayout->addSpacerItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
    btnLayout->addWidget(btnCancel);
    mainLayout->addLayout(btnLayout);

    diffDialog->setLayout(mainLayout);

    int choice = 0;
    
    connect(btnMerge, &QPushButton::clicked, [&]() { choice = 1; diffDialog->accept(); });
    connect(btnReplace, &QPushButton::clicked, [&]() { choice = 2; diffDialog->accept(); });
    connect(btnCancel, &QPushButton::clicked, [&]() { choice = 0; diffDialog->reject(); });

    diffDialog->exec();
    diffDialog->deleteLater();

    if (choice == 0) return;

    if (choice == 2) {
        QFile::remove(externalConfigPath);
        QFile::copy(importPath, externalConfigPath);
    } 
    else if (choice == 1) {
        settings->beginGroup(sectionName);
        for (auto it = backupMap.constBegin(); it != backupMap.constEnd(); ++it) {
            settings->setValue(it.key(), it.value());
        }
        settings->endGroup();
        settings->sync();
    }

    reloadConfigAction();
}


/* 
 * Считывает имя выбранной сетевой карты и запускает процесс 'ip addr show'. 
 * Программно создает модальное диалоговое окно QDialog со стилизованным текстовым полем QTextEdit 
 * в режиме терминала и выводит туда подробную системную информацию об интерфейсе.
 */
void MacChangerWidget::showInterfaceInfo() {
    loadSystemInterfaces();
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
        ui->mac_random_btn->show();
        ui->mac_save_btn->show();
        bool isValid = ui->mac_address_entry->hasAcceptableInput();
        ui->mac_apply_btn->setEnabled(isValid);
        ui->mac_save_btn->setEnabled(isValid);
    } 
    else {
        ui->mac_address_entry->hide(); 
        ui->mac_random_btn->hide();
        ui->mac_save_btn->hide();
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
                    ui->mac_status_lbl->setText("Status: Ready");
                    ui->mac_status_lbl->setProperty("state", "default");
                    ui->mac_status_lbl->style()->unpolish(ui->mac_status_lbl);
                    ui->mac_status_lbl->style()->polish(ui->mac_status_lbl);
                    found = true;
                    break;
                }
            }
        }
        if (!found) {
            ui->mac_current_lbl->setText("Not specified / Dynamic");
            ui->mac_status_lbl->setText("Status: Virtual or unconfigured link detected.");
        }
    } else {
        ui->mac_current_lbl->setText("Interface reading error");
        ui->mac_status_lbl->setText("Status Error: Failed to query interface via system 'ip' tool.");
        ui->mac_status_lbl->setProperty("state", "danger");
        ui->mac_status_lbl->style()->unpolish(ui->mac_status_lbl);
        ui->mac_status_lbl->style()->polish(ui->mac_status_lbl);
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
 * Алгоритм генерации валидного случайного MAC-адреса.
 * Первый байт выбирается из пула чётных (Unicast) и локально администрируемых (Locally Administered),
 * остальные 5 байт генерируются абсолютно случайно.
 */
QString MacChangerWidget::generateRandomMac() {
    static const uint8_t validFirstBytes[] = { 0x02, 0x06, 0x0A, 0x0E, 0x12, 0x16, 0x1A, 0x1E };
    int idx = QRandomGenerator::global()->bounded(0, 8);
    
    QStringList macParts;
    macParts.append(QString("%1").arg(validFirstBytes[idx], 2, 16, QChar('0')).toUpper());
    
    for (int i = 0; i < 5; ++i) {
        int byte = QRandomGenerator::global()->bounded(0, 256);
        macParts.append(QString("%1").arg(byte, 2, 16, QChar('0')).toUpper());
    }
    
    return macParts.join(":");
}

/* Слот для кнопки Rand. Генерирует адрес и вставляет в строку */
void MacChangerWidget::handleRandomMac() {
    QString randomMac = generateRandomMac();
    ui->mac_address_entry->setText(randomMac);
}

/* Слот для кнопки Save. Запрашивает имя и сохраняет профиль в ini файл */
void MacChangerWidget::handleSaveProfile() {
    QString currentMacToSave = ui->mac_address_entry->text().trimmed().toUpper();
    
    if (!macRegex.match(currentMacToSave).hasMatch()) {
        QMessageBox::warning(this, "Error", "Cannot save an invalid MAC address.");
        return;
    }

    bool ok;
    QString profileName = QInputDialog::getText(this, "Save Profile",
                                                "Enter a name for this MAC profile:",
                                                QLineEdit::Normal, "", &ok);
    
    if (!ok || profileName.trimmed().isEmpty()) return;
    
    QString cleanName = profileName.trimmed();
    
    QString diskKey = cleanName.replace(" ", "_");

    settings->beginGroup(sectionName);
    settings->setValue(diskKey, currentMacToSave);
    settings->endGroup();
    settings->sync();

    loadConfig();
    
    int newIndex = ui->mac_alias_dropbox->findText(diskKey);
    if (newIndex != -1) {
        ui->mac_alias_dropbox->setCurrentIndex(newIndex);
    }
    
    QMessageBox::information(this, "Success", QString("Profile '%1' saved successfully!").arg(profileName));
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

    ui->mac_status_lbl->setText("Status: Requesting root privileges via pkexec...");
    ui->mac_status_lbl->setProperty("state", "default");
    ui->mac_status_lbl->style()->unpolish(ui->mac_status_lbl);
    ui->mac_status_lbl->style()->polish(ui->mac_status_lbl);
    QCoreApplication::processEvents();

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
        ui->mac_status_lbl->setText(QString("Status: MAC address changed to %1 on %2!").arg(mac, interface));
        ui->mac_status_lbl->setProperty("state", "success");
        QMessageBox::information(this, "Success", "MAC-address successfully changed!");
        loadSystemInterfaces();
    } else {
        ui->mac_status_lbl->setText("Status Error: Execution failed. Permissions denied or link busy.");
        ui->mac_status_lbl->setProperty("state", "danger");
        QMessageBox::critical(this, "Error", "Execution error. Check the access rights.");
    }
    
    ui->mac_status_lbl->style()->unpolish(ui->mac_status_lbl);
    ui->mac_status_lbl->style()->polish(ui->mac_status_lbl);
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

    connect(newPingTab, &PingTab::networkLossDetected, this, &MacChangerWidget::collectNetworkLossAlert);

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

void MacChangerWidget::startNetworkScan() {
    ui->scan_start_btn->setEnabled(false);
    ui->scan_status_lbl->setText("Scanning network...");
    
    ui->scan_progress->setVisible(true);
    ui->scan_progress->setMaximum(0); 
    ui->scan_progress->setValue(-1);

    ui->scan_table->clearContents();
    ui->scan_table->setRowCount(0);

    if (!m_scanWorker) {
        m_scanWorker = new ScanWorker(this);
        connect(m_scanWorker, &ScanWorker::scanFinished, this, &MacChangerWidget::onScanFinished);
    }
    
    m_scanWorker->start();
}

void MacChangerWidget::onScanFinished(const QVector<DiscoveredDevice> &devices) {
    ui->scan_progress->setVisible(false);
    ui->scan_start_btn->setEnabled(true);
    ui->scan_status_lbl->setText(QString("Found %1 devices.").arg(devices.size()));

    ui->scan_table->setRowCount(devices.size());

    settings->beginGroup(sectionName);

    QMap<QString, QString> macToNameMap;
    for (const QString &key : settings->allKeys()) {
        if (!key.startsWith("HELP_")) {
            QString configMac = settings->value(key).toString().trimmed().toUpper();
            macToNameMap[configMac] = key;
        }
    }
    settings->endGroup();

    for (int i = 0; i < devices.size(); ++i) {
        const auto &dev = devices[i];

        QTableWidgetItem *ipItem = new QTableWidgetItem(dev.ip);
        ipItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        ui->scan_table->setItem(i, 0, ipItem);

        QTableWidgetItem *macItem = new QTableWidgetItem(dev.mac);
        macItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        ui->scan_table->setItem(i, 1, macItem);

        QTableWidgetItem *nameItem = new QTableWidgetItem(""); 
        
        if (macToNameMap.contains(dev.mac)) {
            nameItem->setText(macToNameMap[dev.mac]);
            nameItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            nameItem->setForeground(QBrush(QColor("#888888")));
        } else {
            nameItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
        }
        ui->scan_table->setItem(i, 2, nameItem);
    }
    
    ui->scan_table->resizeColumnsToContents();
}
void MacChangerWidget::saveSelectedAlias() {
    int currentRow = ui->scan_table->currentRow();
    if (currentRow == -1) {
        QMessageBox::warning(this, "Selection Error", "Please select a row in the table first.");
        return;
    }

    QTableWidgetItem *macItem = ui->scan_table->item(currentRow, 1);
    QTableWidgetItem *nameItem = ui->scan_table->item(currentRow, 2);

    if (!macItem || !nameItem) return;

    if (!(nameItem->flags() & Qt::ItemIsEditable)) {
        QMessageBox::information(this, "Info", "This device is already saved in your configuration.");
        return;
    }

    QString enteredName = nameItem->text().trimmed();
    if (enteredName.isEmpty()) {
        QMessageBox::warning(this, "Input Error", "Please enter a name for the device directly in the 'Name' cell.");
        return;
    }

    QString macAddress = macItem->text().toUpper();
    QString diskKey = enteredName.replace(" ", "_");

    settings->beginGroup(sectionName);
    settings->setValue(diskKey, macAddress);
    settings->endGroup();
    settings->sync();

    loadConfig();

    nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
    nameItem->setForeground(QBrush(QColor("#888888")));

    QMessageBox::information(this, "Success", QString("Profile '%1' saved successfully!").arg(enteredName));
}
void MacChangerWidget::changeMacFromScanner() {
    int currentRow = ui->scan_table->currentRow();
    if (currentRow == -1) {
        QMessageBox::warning(this, "Selection Error", "Please select a device from the table first.");
        return;
    }

    QTableWidgetItem *macItem = ui->scan_table->item(currentRow, 1);
    QTableWidgetItem *nameItem = ui->scan_table->item(currentRow, 2);

    if (!macItem || !nameItem) return;

    QString macAddress = macItem->text().toUpper();
    QString deviceName = nameItem->text().trimmed();

    ui->main_tabs->setCurrentIndex(0);

    if (!deviceName.isEmpty() && !(nameItem->flags() & Qt::ItemIsEditable)) {
        int index = ui->mac_alias_dropbox->findText(deviceName);
        if (index != -1) {
            ui->mac_alias_dropbox->setCurrentIndex(index);
        } else {
            ui->mac_alias_dropbox->setCurrentText("[Enter Custom]");
            ui->mac_address_entry->setText(macAddress);
        }
    } else {
        ui->mac_alias_dropbox->setCurrentText("[Enter Custom]");
        ui->mac_address_entry->setText(macAddress);
    }

    bool isValid = ui->mac_address_entry->hasAcceptableInput();
    ui->mac_apply_btn->setEnabled(isValid || ui->mac_alias_dropbox->currentText() != "[Enter Custom]");
    ui->mac_status_lbl->setText(QString("Status: Target configuration loaded from scanner tab (%1)").arg(macAddress));
    ui->mac_status_lbl->setProperty("state", "default");
    ui->mac_status_lbl->style()->unpolish(ui->mac_status_lbl);
    ui->mac_status_lbl->style()->polish(ui->mac_status_lbl);
}


/* Слот перехватывает сигнал падения сети из любого таба */
void MacChangerWidget::collectNetworkLossAlert(const QString &host, const QString &error) {
    if (m_isCentralNotifyLocked) return;

    m_alertCache[host] = error;

    if (!m_centralNotifyTimer->isActive()) {
        m_centralNotifyTimer->start(1000);
    }
}

void MacChangerWidget::sendCentralizedNotification() {
    if (m_alertCache.isEmpty()) return;

    QString hostReport;
    QMap<QString, QString>::const_iterator it = m_alertCache.constBegin();
    while (it != m_alertCache.constEnd()) {
        hostReport.append(QString(" • %1 (%2)\n").arg(it.key(), it.value()));
        ++it;
    }

    if (m_hasTraySupport && trayIcon && trayIcon->isVisible()) {
        trayIcon->showMessage(
            "MacChanger Network Report",
            QString("[!] Connection Lost!\nNo response from following hosts:\n\n%1")
            .arg(hostReport.trimmed()),
            QSystemTrayIcon::Warning,
            6000
        );
    }

    m_alertCache.clear();
    m_isCentralNotifyLocked = true;
    m_centralSpamLockTimer->start(30000);
}

/* Слот снимает блокировку спама через 30 секунд */
void MacChangerWidget::unlockNotificationSpamProtection() {
    m_isCentralNotifyLocked = false;
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
    
    QString binPath = QCoreApplication::applicationFilePath();
    QString currentExecPath = binPath.contains(' ') ? QString("\"%1\" --tray").arg(binPath) : QString("%1 --tray").arg(binPath);

    if (enable) {
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::critical(this, "Autostart Error", 
                "Failed to create the autostart file. Please check folder permissions.");
            
            ui->sett_autostart_switch->blockSignals(true);
            ui->sett_autostart_switch->setChecked(false);
            ui->sett_autostart_switch->blockSignals(false);
            return;
        }

        QTextStream out(&file);
        out.setEncoding(QStringConverter::Utf8);
        
        out << "[Desktop Entry]\n";
        out << "Name=MacChanger ToolKit\n";
        out << "Comment=Fast MAC address changer and network ping toolkit\n";
        out << "Exec=" << currentExecPath << "\n";
        out << "Icon=macchanger-toolkit\n";
        out << "StartupNotify=false\n";
        out << "Terminal=false\n";
        out << "Type=Application\n";
        out << "X-GNOME-Autostart-Phase=Application\n";
        out << "X-KDE-autostart-phase=2\n";
        
        file.close();
    } else {
        if (QFile::exists(filePath)) {
            QFile file(filePath);
            bool safeToDelete = true;
            
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream in(&file);
                while (!in.atEnd()) {
                    QString line = in.readLine();
                    if (line.startsWith("Exec=")) {
                        QString savedExec = line.mid(5).trimmed();
                        if (!savedExec.isEmpty() && savedExec != currentExecPath) {
                            file.close();
                            
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
                            break;
                        }
                    }
                }
                file.close();
            }
            
            QFile::remove(filePath);
        }
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
    TerminalAboutDialog::showAbout(this, "A graphical tool for fast MAC address modification, profile management, and multi-threaded network diagnostics.");
}

void MacChangerWidget::showHelpDialog(){
    TerminalHelpDialog::showHelp(this, "Help");
}