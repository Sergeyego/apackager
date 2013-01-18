#include "progresswidget.h"
#include "ui_progresswidget.h"
#include <QTextEdit>

ProgressWidget::ProgressWidget(QWidget *parent): QDialog(parent), ui(new Ui::ProgressWidgetClass) {
	ui->setupUi(this);
    this->setModal(true);
	ui->totalProgressBar->setMaximum(100);
	connect(ui->cancelButton, SIGNAL(clicked()), this, SLOT(cancelActions()));
    connect(MpkgHandler::getInstance(),SIGNAL(sigProgress(progressMessage)),this,SLOT(updateDataProcessing(progressMessage)));
}

void ProgressWidget::clear()
{
    ui->labelName->setText("");
    ui->labelCurrentActoin->setText("");
    ui->progressBar->setValue(0);
    ui->totalProgressBar->setValue(0);
    ui->cancelButton->setEnabled(true);
}

ProgressWidget::~ProgressWidget() {
}

void ProgressWidget::updateDataProcessing(progressMessage message) {
    ui->labelName->setText(message.name);
    ui->labelCurrentActoin->setText(message.currentAction);
    if (message.progress>=0 && message.progress<=100) ui->progressBar->setValue(message.progress);
    else if (message.progress<0) ui->progressBar->setValue(0);
    if (message.totalProgress>=0 && message.totalProgress<=100) ui->totalProgressBar->setValue(message.totalProgress);
    else if (message.totalProgress<0) ui->totalProgressBar->setValue(0);
}

void ProgressWidget::cancelActions() {
	if (QMessageBox::warning(this, tr("Please confirm abort"), tr("Are you sure you want to abort current operations?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No)==QMessageBox::Yes) {
        emit abort(true);
        ui->cancelButton->setEnabled(false);
    }
}

void ProgressWidget::closeEvent(QCloseEvent *event)
{
    event->ignore();
}

MpkgErrorBox::MpkgErrorBox(QWidget *parent):QMessageBox(parent)
{
    this->setWindowTitle(tr("Error"));
    this->setIcon(QMessageBox::Critical);
    connect(MpkgHandler::getInstance(),SIGNAL(sigErr(errorMessage)),this,SLOT(showError(errorMessage)));
    connect(this,SIGNAL(sigButtonClick(int)),MpkgHandler::getInstance(),SLOT(clickButtonSlot(int)));
}

void MpkgErrorBox::showError(errorMessage message)
{
    this->setText(message.error);
    this->setInformativeText(message.informativeText);

    for (int i=0; i<this->buttons().size(); ++i){
        this->removeButton(this->buttons()[i]);
    }

    if (!message.textButtons.size()) {
        this->addButton(QMessageBox::Ok);
    } else {
        for (int i=0; i<message.textButtons.size(); ++i){
            this->addButton(new QPushButton(message.textButtons[i]), QMessageBox::ActionRole);
        }
    }

    this->exec();
    int but=0;
    for (int i=0; i<this->buttons().size(); ++i){
        if (this->clickedButton()==buttons()[i]) but=i;
    }
    emit sigButtonClick(but);
}

