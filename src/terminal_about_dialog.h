#pragma once

#include <QDialog>
#include <QString>

class TerminalAboutDialog : public QDialog {
    Q_OBJECT

public:
    static void showAbout(QWidget *parent, const QString &description);

private:
    explicit TerminalAboutDialog(QWidget *parent, const QString &description);
protected:
    void keyPressEvent(QKeyEvent *event) override;
};
