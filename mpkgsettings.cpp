#include "mpkgsettings.h"
#include "ui_mpkgsettings.h"

MpkgSettings::MpkgSettings(MpkgEngine *engine, CategoryModel *model, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MpkgSettings)
{
    ui->setupUi(this);
    mpkg=engine;
    categoryModel=model;
    proxyCategoryModel = new ProxyCategoryModel(this);
    proxyCategoryModel->setSourceModel(categoryModel);
    ui->listWidgetSettingsCategory->addItem(new QListWidgetItem(QIcon(":/icons/internet.png"),tr("Repositories")));
    ui->listWidgetSettingsCategory->addItem(new QListWidgetItem(QIcon(":/icons/settings.png"),tr("Mpkg core settings")));
    ui->listWidgetSettingsCategory->addItem(new QListWidgetItem(QIcon(":/icons/list.png"),tr("Update blacklist")));
    ui->listWidgetSettingsCategory->addItem(new QListWidgetItem(QIcon(":/icons/list.png"),tr("Remove blacklist")));
    ui->listWidgetSettingsCategory->addItem(new QListWidgetItem(QIcon(":/icons/accessories.png"),tr("Categories")));
    ui->listViewCategories->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->listViewCategories->setModel(proxyCategoryModel);
    ui->listViewCategories->setModelColumn(3);
    ui->listWidgetSettingsCategory->setCurrentRow(0);
    QStringList downloadTools;
    downloadTools<<"aria2"<<"mpkg"<<"wget";
    ui->comboBoxDownloadTool->addItems(downloadTools);
    load();
    connect(ui->listWidgetSettingsCategory,SIGNAL(currentRowChanged(int)),ui->stackedWidget,SLOT(setCurrentIndex(int)));
    connect(ui->cmdAddRepository,SIGNAL(clicked()),this,SLOT(addRepository()));
    connect(ui->cmdInternetRep,SIGNAL(clicked()),mpkg,SLOT(downloadRepositoryList()));
    connect(mpkg,SIGNAL(sigUpdRep()),this,SLOT(refreshRepView()));
    connect(ui->cmdAddUpdateBlacklist,SIGNAL(clicked()),this,SLOT(addUpdateBlacklist()));
    connect(ui->cmdAddRemoveBlacklist,SIGNAL(clicked()),this,SLOT(addRemoveProtected()));
    connect(ui->cmdAddCategory,SIGNAL(clicked()),this,SLOT(addCategory()));
    connect(ui->cmdRemoveCaterory,SIGNAL(clicked()),this,SLOT(removeCategory()));
    connect(ui->cmdEditCategory,SIGNAL(clicked()),this,SLOT(edtCategory()));
    connect(ui->listViewCategories,SIGNAL(doubleClicked(QModelIndex)),this,SLOT(edtCategory()));
}

MpkgSettings::~MpkgSettings()
{
    delete ui;
}

void MpkgSettings::keyPressEvent(QKeyEvent *p)
{
    p->ignore();
}

void MpkgSettings::save()
{
    mpkg->setConfigValue("disable_dependencies",toStr(ui->checkBoxDisableDependencies->isChecked()));
    mpkg->setConfigValue("enable_prelink",toStr(ui->checkBoxEnablePrelink->isChecked()));
    mpkg->setConfigValue("enable_prelink_randomization",toStr(ui->checkBoxEnablePrelinkRandom->isChecked()));
    mpkg->setConfigValue("download_tool",ui->comboBoxDownloadTool->currentText());
    mpkg->setConfigValue("cdrom_device",ui->lineEditCdRomDevice->text());
    mpkg->setConfigValue("cdrom_mountpoint",ui->lineEditCdRomMountPoint->text());
    QStringList enabledRep, disabledRep;
    for (int i=0; i<ui->listWidgetRepositories->count(); ++i){
        if (ui->listWidgetRepositories->item(i)->checkState()==Qt::Checked)
            enabledRep.push_back(ui->listWidgetRepositories->item(i)->text());
        else disabledRep.push_back(ui->listWidgetRepositories->item(i)->text());
    }
    mpkg->setRepositoryList(enabledRep, disabledRep);
    QStringList updateBL, removeBL;
    for (int i=0; i<ui->listWidgetUpdateBlacklist->count(); ++i)
        updateBL.push_back(ui->listWidgetUpdateBlacklist->item(i)->text());
    for (int i=0; i<ui->listWidgetRemoveBlacklist->count(); ++i)
        removeBL.push_back(ui->listWidgetRemoveBlacklist->item(i)->text());
    mpkg->setBlacklists(updateBL, removeBL);
    categoryModel->commit();
}

void MpkgSettings::refreshRepView()
{
    QStringList enabled, disabled;
    mpkg->getRepositoryList(enabled, disabled);
    ui->listWidgetRepositories->clear();
    for (unsigned int i=0; i<enabled.size(); ++i) {
        QListWidgetItem *item = new QListWidgetItem(enabled[i]);
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
        item->setCheckState(Qt::Checked);
        ui->listWidgetRepositories->addItem(item);
    }
    for (unsigned int i=0; i<disabled.size(); ++i) {
        QListWidgetItem *item = new QListWidgetItem(disabled[i]);
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
        item->setCheckState(Qt::Unchecked);
        ui->listWidgetRepositories->addItem(item);
    }
}

void MpkgSettings::addRemoveProtected()
{
    if (ui->lineEditAddRemoveBlacklist->text().isEmpty()) return;
    ui->listWidgetRemoveBlacklist->addItem(ui->lineEditAddRemoveBlacklist->text());
    ui->lineEditAddRemoveBlacklist->clear();
    ui->listWidgetRemoveBlacklist->scrollToBottom();
}

void MpkgSettings::addUpdateBlacklist()
{
    if (ui->lineEditAddUpdateBlacklist->text().isEmpty()) return;
    ui->listWidgetUpdateBlacklist->addItem(ui->lineEditAddUpdateBlacklist->text());
    ui->lineEditAddUpdateBlacklist->clear();
    ui->listWidgetUpdateBlacklist->scrollToBottom();
}

void MpkgSettings::addCategory()
{
    categoryModel->insertRow();
    ui->listViewCategories->scrollToBottom();
    ui->listViewCategories->setCurrentIndex(ui->listViewCategories->model()->index(ui->listViewCategories->model()->rowCount()-1,3));
}

void MpkgSettings::removeCategory()
{
    int row=proxyCategoryModel->mapToSource(ui->listViewCategories->currentIndex()).row();
    if (row<0 || row>=categoryModel->rowCount()) return;
    int n=QMessageBox::question(NULL,tr("Remove category"),
                                  tr("Remove ")+categoryModel->data(categoryModel->index(row,3)).toString()+tr("?"),QMessageBox::Yes| QMessageBox::No);
    if (n==QMessageBox::Yes){
        categoryModel->removeRow(row);
    }
}

void MpkgSettings::edtCategory()
{
    QStringList tagsList=mpkg->availableTags();
    int index=proxyCategoryModel->mapToSource(ui->listViewCategories->currentIndex()).row();
    if (index<0 || index>=categoryModel->rowCount()) return;
    EdtCategoryDialog dialog(categoryModel,index,tagsList);
    dialog.exec();
}

void MpkgSettings::load()
{
    refreshRepView();
    ui->checkBoxDisableDependencies->setChecked(mpkg->getConfigValue("disable_dependencies")=="yes");
    ui->checkBoxEnablePrelink->setChecked(mpkg->getConfigValue("enable_prelink")=="yes");
    ui->checkBoxEnablePrelinkRandom->setChecked(mpkg->getConfigValue("enable_prelink_randomization")=="yes");
    ui->lineEditCdRomDevice->setText(mpkg->getConfigValue("cdrom_device"));
    ui->lineEditCdRomMountPoint->setText(mpkg->getConfigValue("cdrom_mountpoint"));
    QString downloadTool=mpkg->getConfigValue("download_tool");
    if (downloadTool=="aria2c") downloadTool="aria2";
    ui->comboBoxDownloadTool->setCurrentIndex(ui->comboBoxDownloadTool->findText(downloadTool));
    ui->listWidgetUpdateBlacklist->addItems(mpkg->updateBlackList());
    ui->listWidgetRemoveBlacklist->addItems(mpkg->removeBlacklist());
}

QString MpkgSettings::toStr(bool val)
{
    return val? "yes" : "no";
}

void MpkgSettings::addRepository()
{
    if (ui->lineEditAddRepository->text().isEmpty()) return;
    QListWidgetItem *item = new QListWidgetItem(ui->lineEditAddRepository->text());
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
    item->setCheckState(Qt::Checked);
    ui->listWidgetRepositories->addItem(item);
    ui->lineEditAddRepository->clear();
    ui->listWidgetRepositories->scrollToBottom();
}


SettingsListWidget::SettingsListWidget(QWidget *parent)
    :QListWidget(parent)
{
    removeAction = new QAction(QIcon(":/icons/remove.png"),tr("Delete"),this);
    connect(removeAction,SIGNAL(triggered()),this,SLOT(removeCurrentItem()));
}

void SettingsListWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Delete) removeCurrentItem();
    QListWidget::keyPressEvent(event);
}

void SettingsListWidget::contextMenuEvent(QContextMenuEvent *event)
{
    if (this->count()){
        QMenu menu(this);
        menu.addAction(removeAction);
        menu.exec(event->globalPos());
    }
}

void SettingsListWidget::removeCurrentItem()
{
    int row=this->currentRow();
    if(row>=0 && row<this->count())
        this->takeItem(row);
}

