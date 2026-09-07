#include "terminal_help_dialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QPushButton>
#include <QListWidget>
#include <QFile>
#include <QTextStream>
#include <QCoreApplication>
#include <QSysInfo>
#include <QProcessEnvironment>
#include <QKeyEvent>

TerminalHelpDialog::TerminalHelpDialog(QWidget *parent, const QString &title)
    : QDialog(parent)
{
    setWindowTitle(QString("system_info --%1").arg(title.toLower().replace(" ", "_")));
    setMinimumSize(720, 460);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    QHBoxLayout *contentLayout = new QHBoxLayout();

    QListWidget *menuList = new QListWidget(this);
    menuList->setFixedWidth(180);
    menuList->setStyleSheet(
        "QListWidget {"
        "    background-color: #111111;"
        "    border: 1px solid #222222;"
        "    color: #ff003c;"
        "    font-family: monospace;"
        "    font-weight: bold;"
        "    font-size: 11px;"
        "    padding: 5px;"
        "}"
        "QListWidget::item {"
        "    padding: 6px 3px;"
        "    border-bottom: 1px solid #1a1a1a;"
        "}"
        "QListWidget::item:hover {"
        "    background-color: #1e1e1e;"
        "    color: #00C3FF;"
        "}"
        "QListWidget::item:selected {"
        "    background-color: #222222;"
        "    color: #00C3FF;"
        "    border-left: 2px solid #00C3FF;"
        "}"
    );

    menuList->addItem(" MAC Spoofer Logic");
    menuList->addItem(" Network Scanner");
    menuList->addItem(" Telemetry Engine");
    menuList->addItem(" Operation TL;DR");

    contentLayout->addWidget(menuList);

    QTextEdit *txtHelp = new QTextEdit(this);
    txtHelp->setReadOnly(true);
    txtHelp->setUndoRedoEnabled(false);
    txtHelp->setStyleSheet(
        "QTextEdit {"
        "    background-color: #0c0c0c;"
        "    border: 1px solid #222222;"
        "    font-family: 'Source Code Pro', 'Fira Code', 'Courier New', monospace;"
        "    font-size: 12px;"
        "    color: #ffffff;"
        "    padding: 8px;"
        "}"
    );
    contentLayout->addWidget(txtHelp);
    mainLayout->addLayout(contentLayout);

    QString systemUser = QProcessEnvironment::systemEnvironment().value("USER", "user");
    QString systemHost = QSysInfo::machineHostName();
    if (systemHost.isEmpty()) systemHost = "linux";
    
    QString promptTop = QString("<span style='color: #ff7675;'>╭─</span>"
                                 "<span style='color: #ff003c;'>%1</span>"
                                 "<span style='color: #ffffff;'>@</span>"
                                 "<span style='color: #ff003c;'>%2</span> "
                                 "<span style='color: #ffffff;'>in</span> "
                                 "<span style='color: #ffffff;'>~</span> "
                                 "<span style='color: #ffffff;'>took</span> "
                                 "<span style='color: #f1c40f;'>0s</span><br>")
                         .arg(systemUser, systemHost);

    QMap<int, QString> helpRoutes;
    helpRoutes[0] = ":/help_mac.md";
    helpRoutes[1] = ":/help_scan.md";
    helpRoutes[2] = ":/help_ping.md";
    helpRoutes[3] = ":/help_tldr.md";

    auto loadHelpSection = [&](int index) {
        if (!helpRoutes.contains(index)) return;

        QString cmdName;
        if (index == 0) cmdName = "mac_spoofer_core";
        else if (index == 1) cmdName = "net_recon_sweep";
        else if (index == 2) cmdName = "telemetry_daemon";
        else cmdName = "operation_tldr";

        QString execPrompt = promptTop + QString("<span style='color: #ff7675;'>╰─λ</span> ./%1 --show-%2<br><br>")
                                      .arg(QCoreApplication::applicationName().toLower().replace(" ", "_"), cmdName);

        QString waitingPrompt = QString("<br><br><span style='color: #ff7675;'>╭─</span>"
                                        "<span style='color: #ff003c;'>%1</span>"
                                        "<span style='color: #ffffff;'>@</span>"
                                        "<span style='color: #ff003c;'>%2</span> "
                                        "<span style='color: #ffffff;'>in</span> "
                                        "<span style='color: #ffffff;'>~</span><br>"
                                        "<span style='color: #ff7675;'>╰─λ</span> <span style='color: #ffffff; background-color: #ffffff;'>&nbsp;</span>")
                                .arg(systemUser, systemHost);

        QFile file(helpRoutes[index]);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            txtHelp->setMarkdown(in.readAll());
            file.close();
            
            QString parsedHtml = txtHelp->toHtml();
            txtHelp->setHtml(execPrompt + parsedHtml + waitingPrompt);
        }
    };

    connect(menuList, &QListWidget::currentRowChanged, this, loadHelpSection);
    menuList->setCurrentRow(0);

    QPushButton *btnClose = new QPushButton("exit", this);
    btnClose->setCursor(Qt::PointingHandCursor);
    btnClose->setStyleSheet(
        "QPushButton { background-color: #1e1e1e; color: #ff7675; border: 1px solid #333333; font-family: monospace; padding: 5px 15px; }"
        "QPushButton:hover { background-color: #2a2a2a; border-color: #ff003c; color: #ffffff; }"
    );
    connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);
    
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    btnLayout->addWidget(btnClose);
    mainLayout->addLayout(btnLayout);

    setAttribute(Qt::WA_DeleteOnClose);
}

void TerminalHelpDialog::keyPressEvent(QKeyEvent *event) {
    if (event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_D) {
        event->accept();
        this->accept();
        return;
    }
    QDialog::keyPressEvent(event);
}

void TerminalHelpDialog::showHelp(QWidget *parent, const QString &title) {
    TerminalHelpDialog *dialog = new TerminalHelpDialog(parent, title);
    dialog->exec();
}
