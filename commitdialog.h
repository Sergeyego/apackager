#ifndef COMMITDIALOG_H
#define COMMITDIALOG_H

#include <QDialog>

namespace Ui {
class CommitDialog;
}

class CommitDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit CommitDialog(QStringList &install, QStringList &remove, QStringList &upgrade, QString &info, QWidget *parent = 0);
    ~CommitDialog();
    
private:
    Ui::CommitDialog *ui;
};

#endif // COMMITDIALOG_H
