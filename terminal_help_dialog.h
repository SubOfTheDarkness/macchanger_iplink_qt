#pragma once

#include <QDialog>
#include <QString>

class TerminalHelpDialog : public QDialog {
    Q_OBJECT

public:
    static void showHelp(QWidget *parent, const QString &title);

private:
    explicit TerminalHelpDialog(QWidget *parent, const QString &title);

protected:
    void keyPressEvent(QKeyEvent *event) override;
};
