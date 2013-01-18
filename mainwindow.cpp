#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindowClass)
{
    ui->setupUi(this);

    ui->toolBar->addAction(ui->actionCommit);
    ui->toolBar->addSeparator();
    ui->toolBar->addAction(ui->actionUpdate_repository_data);
    ui->toolBar->addSeparator();
    ui->toolBar->addAction(ui->actionSettings);

    loadSettings();

    progress = new ProgressWidget(this);
    errorBox = new MpkgErrorBox(this);
    engine = new MpkgEngine(this);
    mapper = new QDataWidgetMapper(this);

    if (!engine->lockDataBase()) {
        QMessageBox::critical(this, tr("Error locking database"), tr("Cannot lock database because it is locked by another process"), QMessageBox::Ok);
        exit(1);
    }

    updateRepositoryThread = new UpdateRepository(engine);
    commitThread = new Commit(engine);
    loadPackageThread = new LoadPackage(engine);

    pkgModel = new MpkgModel(engine,this);
    mapper->setModel(pkgModel);
    mapper->addMapping(ui->textEditPkgInfo,5);
    mapper->addMapping(ui->textEditDependencies,6);
    pkgProxyModel = new MpkgProxyModel(this);
    pkgProxyModel->setBlacklist(engine->updateBlackList());
    pkgProxyModel->setSourceModel(pkgModel);
    pkgSearchModel = new MpkgSearchModel(this);
    pkgSearchModel->setSourceModel(pkgProxyModel);

    ui->pkgView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->pkgView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->pkgView->verticalHeader()->setDefaultSectionSize(ui->pkgView->verticalHeader()->fontMetrics().height()*1.5);
    ui->pkgView->verticalHeader()->hide();

    tagsModel = new TagsModel(engine,this);
    categoryModel = new CategoryModel(this);
    sortCategoryModel = new QSortFilterProxyModel(this);
    sortCategoryModel->setSourceModel(categoryModel);
    sortCategoryModel->sort(0);
    setTagsModel();
    ui->comboBoxFilter->addItems(pkgModel->headerList());

    connect(ui->cmdChkAll,SIGNAL(clicked()),this,SLOT(selectAll()));
    connect(ui->cmdUchkAll,SIGNAL(clicked()),this,SLOT(unSelectAll()));
    connect(ui->cmdResetAll,SIGNAL(clicked()),this,SLOT(resetSelect()));
    connect(ui->lineEditFilter,SIGNAL(textChanged(QString)),this,SLOT(pkgTextFilter(QString)));
    connect(ui->cmdClearFilter,SIGNAL(clicked()),ui->lineEditFilter,SLOT(clear()));
    connect(ui->comboBoxFilter,SIGNAL(currentIndexChanged(QString)),this,SLOT(chFilterColumn()));
    connect(ui->actionSettings,SIGNAL(triggered()),this,SLOT(showSettings()));
    connect(ui->actionPackage_info,SIGNAL(triggered(bool)),ui->dockWidgetPackageInfo,SLOT(setVisible(bool)));
    connect(ui->actionDependency,SIGNAL(triggered(bool)),ui->dockWidgetDependency,SLOT(setVisible(bool)));
    connect(ui->actionReset_queue,SIGNAL(triggered()),this,SLOT(resetQueue()));
    connect(ui->dockWidgetPackageInfo,SIGNAL(visibilityChanged(bool)),ui->actionPackage_info,SLOT(setChecked(bool)));
    connect(ui->dockWidgetDependency,SIGNAL(visibilityChanged(bool)),ui->actionDependency,SLOT(setChecked(bool)));
    connect(engine,SIGNAL(updateFinished()),this,SLOT(updateFinishedSlot()));
    connect(ui->actionCommit,SIGNAL(triggered()),this,SLOT(commitSlot()));
    connect(ui->cmdCommit,SIGNAL(clicked()),this,SLOT(commitSlot()));
    connect(ui->actionUpdate_repository_data,SIGNAL(triggered()),this,SLOT(updateRepositoryDataSlot()));
    connect(ui->actionReload_package_list,SIGNAL(triggered()),this,SLOT(updateSlot()));
    connect(ui->actionClean_cache,SIGNAL(triggered()),this,SLOT(cleanCache()));
    connect(ui->actionQuit,SIGNAL(triggered()),this,SLOT(close()));
    connect(engine,SIGNAL(updateRepositoryDataFinished()),this,SLOT(updateRepositoryDataFinishedSlot()));
    connect(engine,SIGNAL(commitFinished()),this,SLOT(commitFinishedSlot()));
    connect(progress,SIGNAL(abort(bool)),engine,SLOT(setAbortActions(bool)));
    connect(ui->actionHide_deprecated,SIGNAL(triggered(bool)),pkgProxyModel,SLOT(setHideDeprecated(bool)));
    connect(ui->actionHide_installed,SIGNAL(triggered(bool)),pkgProxyModel,SLOT(setHideInstalled(bool)));
    connect(ui->actionHide_not_installed,SIGNAL(triggered(bool)),pkgProxyModel,SLOT(setHideNotInstalled(bool)));
    connect(ui->actionHide_installed_deprecated,SIGNAL(triggered(bool)),pkgProxyModel,SLOT(setHideInstalledDeprecated(bool)));
    connect(ui->actionHide_updated,SIGNAL(triggered(bool)),pkgProxyModel,SLOT(setHideUpdated(bool)));
    connect(ui->radioButtonTags,SIGNAL(clicked()),this,SLOT(setTagsModel()));
    connect(ui->radioButtonCategory,SIGNAL(clicked()),this,SLOT(setTagsModel()));
    connect(engine,SIGNAL(sigError(QString,QString,QString)),this,SLOT(showError(QString,QString,QString)));
    connect(ui->actionToolbar,SIGNAL(triggered(bool)),ui->toolBar,SLOT(setVisible(bool)));
    connect(ui->toolBar,SIGNAL(visibilityChanged(bool)),ui->actionToolbar,SLOT(setChecked(bool)));
    connect(ui->actionOpen,SIGNAL(triggered()),this,SLOT(openFile()));
    connect(ui->actionAbout,SIGNAL(triggered()),this,SLOT(about()));
    connect(ui->comboBoxDistr,SIGNAL(currentIndexChanged(int)),this,SLOT(setRepFilter(int)));
}

MainWindow::~MainWindow()
{
    delete ui;
    saveSettings();
}

void MainWindow::about()
{
    QString text="<b><big>Apackager</b></big> version 1.0 beta2 <br>"+tr("AgiliaLinux package management system: GUI front-end")+
            "<br>"+tr("Licensed under GPLv2")+"<br>"+ tr("Authors:")+" sergen, aix27249";
    QMessageBox::about(this,tr("about"),text);
}

void MainWindow::installFiles(QStringList &files)
{
    if (!files.isEmpty()){
        if (engine->createQueue(files)){
            runCommit();
        } else updateSlot();
    }
}

void MainWindow::loadSettings() {
    QSettings settings("MpkgGUI", "mpkgmanager");
    this->restoreGeometry(settings.value("geometry").toByteArray());
    this->restoreState(settings.value("state").toByteArray());
    this->ui->splitter->restoreState(settings.value("widthCat").toByteArray());
    ui->actionPackage_info->setChecked(!ui->dockWidgetPackageInfo->isHidden());
    ui->actionDependency->setChecked(!ui->dockWidgetDependency->isHidden());
    ui->actionToolbar->setChecked(!ui->toolBar->isHidden());
}

void MainWindow::saveSettings() {
    QSettings settings("MpkgGUI", "mpkgmanager");
    settings.setValue("state", this->saveState());
    settings.setValue("geometry", this->saveGeometry());
    settings.setValue("width0",ui->pkgView->columnWidth(0));
    settings.setValue("width1",ui->pkgView->columnWidth(1));
    settings.setValue("width2",ui->pkgView->columnWidth(2));
    settings.setValue("widthCat",ui->splitter->saveState());
}

QModelIndex MainWindow::sourcePkgIndex(QModelIndex index)
{
    QModelIndex proxyModelIndex= pkgSearchModel->mapToSource(index);
    return pkgProxyModel->mapToSource(proxyModelIndex);
}

void MainWindow::runCommit()
{
    QStringList install, remove, upgrade;
    QString info;
    info=engine->createCommitLists(install,remove,upgrade);
    CommitDialog commit(install,remove,upgrade,info);
    if (commit.exec()==QDialog::Accepted){
        ui->statusBar->showMessage(tr("Committing action"));
        progress->clear();
        progress->show();
        commitThread->start();
    }
}

void MainWindow::pkgTagsFilter(QModelIndex tagIndex)
{
    QStringList taglist=ui->tagView->model()->data(ui->tagView->model()->index(tagIndex.row(),1)).toStringList();
    QStringList statelist=ui->tagView->model()->data(ui->tagView->model()->index(tagIndex.row(),2)).toStringList();
    pkgProxyModel->setFilter(taglist,statelist);
}

void MainWindow::selectAll()
{
    for (unsigned int i=0; i<pkgSearchModel->rowCount(); ++i){
        QModelIndex ind=pkgSearchModel->index(i,0);
        pkgSearchModel->setData(ind,true,Qt::CheckStateRole);
    }
}

void MainWindow::unSelectAll()
{
    for (unsigned int i=0; i<pkgSearchModel->rowCount(); ++i){
        QModelIndex ind=pkgSearchModel->index(i,0);
        pkgSearchModel->setData(ind,false,Qt::CheckStateRole);
    }
}

void MainWindow::resetSelect()
{
    for (unsigned int i=0; i<pkgSearchModel->rowCount(); ++i){
        int sourceRow=sourcePkgIndex(pkgSearchModel->index(i,0)).row();
        int state=pkgModel->data(pkgModel->index(sourceRow,4)).toInt();
        bool value=(state==ICONSTATE_INSTALLED)||(state==ICONSTATE_INSTALLED_DEPRECATED);
        pkgSearchModel->setData(pkgSearchModel->index(i,0),value,Qt::CheckStateRole);
    }
}

void MainWindow::pkgTextFilter(QString text)
{
    pkgSearchModel->setFilterKeyColumn(ui->comboBoxFilter->currentIndex());
    pkgSearchModel->setFilterFixedString(text);
}

void MainWindow::chFilterColumn()
{
    pkgTextFilter(ui->lineEditFilter->text());
    ui->lineEditFilter->setFocus();
}

void MainWindow::showSettings()
{
    MpkgSettings settings(engine, categoryModel);
    if (settings.exec()==QDialog::Accepted){
        settings.save();
        pkgProxyModel->setBlacklist(engine->updateBlackList());
    } else {
        categoryModel->refresh();
    }
}

void MainWindow::updateSlot()
{
    loadPackageThread->start();
    ui->toolBar->setEnabled(false);
    ui->menuBar->setEnabled(false);
    ui->centralWidget->setEnabled(false);
    ui->statusBar->showMessage(tr("Update package list..."));
}

void MainWindow::updateFinishedSlot()
{
    if (!ui->pkgView->model()) {
        ui->pkgView->setModel(pkgSearchModel);
        connect(ui->pkgView->selectionModel(),SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),this,SLOT(refreshPkgInfo(QModelIndex)));
        QSettings settings("MpkgGUI", "mpkgmanager");
        ui->pkgView->setColumnWidth(0,settings.value("width0").toInt()? settings.value("width0").toInt() : 200);
        ui->pkgView->setColumnWidth(1,settings.value("width1").toInt()? settings.value("width1").toInt() : 100);
        ui->pkgView->setColumnWidth(2,settings.value("width2").toInt()? settings.value("width2").toInt() : 200);
    }
    ui->comboBoxDistr->clear();
    ui->comboBoxDistr->addItem(tr("All repositories"));
    ui->comboBoxDistr->addItems(engine->pkgDistrList());
    ui->toolBar->setEnabled(true);
    ui->centralWidget->setEnabled(true);
    ui->menuBar->setEnabled(true);
    ui->statusBar->clearMessage();
}

void MainWindow::updateRepositoryDataSlot()
{
    ui->statusBar->showMessage(tr("Updating repository data"));
    progress->clear();
    progress->show();
    updateRepositoryThread->start();
}

void MainWindow::updateRepositoryDataFinishedSlot()
{
    ui->statusBar->clearMessage();
    updateSlot();
    progress->hide();
}

void MainWindow::commitSlot()
{
    if (pkgModel->actionsIsEmpty()) {
        QMessageBox::information(this, tr("Nothing to perform"), tr("No actions to perform"));
        return;
    }
    if (pkgModel->refreshQueue())
        runCommit();
}

void MainWindow::commitFinishedSlot()
{
    ui->statusBar->clearMessage();
    updateSlot();
    progress->hide();
}

void MainWindow::refreshPkgInfo(QModelIndex pkgIndex)
{
    mapper->setCurrentIndex(sourcePkgIndex(pkgIndex).row());
}

void MainWindow::resetQueue()
{
    ui->statusBar->showMessage(tr("Cleaning queue"));
    engine->resetQueue();
    QMessageBox::information(this, tr("Queue cleaned up"), tr("Action queue was cleaned up"), QMessageBox::Ok);
    pkgModel->refresh();
    ui->statusBar->clearMessage();
}

void MainWindow::cleanCache()
{
    ui->statusBar->showMessage(tr("Cleaning cache"));
    engine->cleanCache();
    QMessageBox::information(this, tr("Cache cleaned up"), tr("Package cache was cleaned up"), QMessageBox::Ok);
    ui->statusBar->clearMessage();
}

void MainWindow::setTagsModel()
{
    if (ui->tagView->model())
        disconnect(ui->tagView->selectionModel(),SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),this,SLOT(pkgTagsFilter(QModelIndex)));
    if (ui->radioButtonTags->isChecked()){
        ui->tagView->setModel(tagsModel);
    } else if (ui->radioButtonCategory->isChecked()) {
       ui->tagView->setModel(sortCategoryModel);
    }
    if (ui->tagView->model())
        connect(ui->tagView->selectionModel(),SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),this,SLOT(pkgTagsFilter(QModelIndex)));
}

void MainWindow::showError(QString title, QString text, QString details)
{
    QMessageBox err;
    err.setWindowTitle(title);
    err.setIcon(QMessageBox::Critical);
    err.setText(text);
    err.setDetailedText(details);
    err.exec();
}

void MainWindow::openFile()
{
    QStringList files=QFileDialog::getOpenFileNames(this, tr("Open File"),"/",tr("Packages *.txz, *.tgz (*.txz, *.tgz)"));
    installFiles(files);
}

void MainWindow::setRepFilter(int comboBoxIndex)
{
    QString filter=comboBoxIndex? ui->comboBoxDistr->currentText() : QString();
    pkgProxyModel->setRepositoryFilter(filter);
}
