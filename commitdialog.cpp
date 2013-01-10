#include "commitdialog.h"
#include "ui_commitdialog.h"

CommitDialog::CommitDialog(QStringList &install, QStringList &remove, QStringList &upgrade, QString &info, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CommitDialog)
{
    ui->setupUi(this);
    ui->actionSummaryLabel->setText(info);
    ui->installListWidget->addItems(install);
    ui->removeListWidget->addItems(remove);
    ui->upgradeListWidget->addItems(upgrade);
}

CommitDialog::~CommitDialog()
{
    delete ui;
}
