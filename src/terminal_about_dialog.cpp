#include "terminal_about_dialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QPushButton>
#include <QKeyEvent>
#include <QSpacerItem>
#include <QCoreApplication>
#include <QSysInfo>
#include <QProcessEnvironment>

TerminalAboutDialog::TerminalAboutDialog(QWidget *parent, const QString &description)
    : QDialog(parent)
{
    setWindowTitle("system_info --about");
    setMinimumSize(540, 390);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);

    QTextEdit *txtAbout = new QTextEdit(this);
    txtAbout->setReadOnly(true);
    
    txtAbout->setStyleSheet(
        "QTextEdit {"
        "    background-color: #0c0c0c;"
        "    border: 1px solid #222222;"
        "    font-family: 'Source Code Pro', 'Fira Code', 'Courier New', monospace;"
        "    font-size: 12px;"
        "}"
    );

    QString globalAppName = QCoreApplication::applicationName();
    if (globalAppName.isEmpty()) globalAppName = "Unknown Application";

    QString globalVersion = QCoreApplication::applicationVersion();
    if (globalVersion.isEmpty()) globalVersion = "1.0.0";

    QString systemUser = QProcessEnvironment::systemEnvironment().value("USER");
    if (systemUser.isEmpty()) systemUser = "user";
    
    QString systemHost = QSysInfo::machineHostName();
    if (systemHost.isEmpty()) systemHost = "linux";

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
        "%2 ./%3 --about<br><br>"
        "--------------------------------------------------<br>"
        "<span style='color: #00FF66;'>▶ APPLICATION:</span> %4<br>"
        "<span style='color: #00FF66;'>▶ VERSION:    </span> v%5<br>"
        "<span style='color: #00FF66;'>▶ DEVELOPER:  </span> SubOfTheDarkness<br>"
        "<span style='color: #00FF66;'>▶ DESCRIPTION:</span> %6<br>"
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
    ).arg(promptTop, promptBottom, globalAppName.toLower().replace(" ", "_"), globalAppName, globalVersion, description);

    txtAbout->setHtml(htmlContent);
    layout->addWidget(txtAbout);

    QPushButton *btnClose = new QPushButton("exit", this);
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
    connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);
    
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
    btnLayout->addWidget(btnClose);
    layout->addLayout(btnLayout);

    setAttribute(Qt::WA_DeleteOnClose);
}

void TerminalAboutDialog::keyPressEvent(QKeyEvent *event) {
    if (event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_D) {
        event->accept();
        this->accept();
        return;
    }
    
    QDialog::keyPressEvent(event);
}


void TerminalAboutDialog::showAbout(QWidget *parent, const QString &description) {
    TerminalAboutDialog *dialog = new TerminalAboutDialog(parent, description);
    dialog->exec();
}
