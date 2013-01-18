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
    explicit CommitDialog(const QStringList &install, const QStringList &remove, const QStringList &upgrade, const QString &info, QWidget *parent = 0);
    ~CommitDialog();
    
private:
    Ui::CommitDialog *ui;
};

#endif // COMMITDIALOG_H
