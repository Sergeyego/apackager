#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui>
#include "mpkgmodel.h"
#include "mpkgengine.h"
#include "commitdialog.h"
#include "progresswidget.h"
#include "mpkgthread.h"
#include <QDataWidgetMapper>
#include "mpkgsettings.h"

namespace Ui {
    class MainWindowClass;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void installFiles(QStringList &files);
    
private:
    Ui::MainWindowClass *ui;
    MpkgModel *pkgModel;
    MpkgProxyModel *pkgProxyModel;
    MpkgSearchModel *pkgSearchModel;
    MpkgEngine *engine;
    ProgressWidget *progress;
    MpkgErrorBox *errorBox;
    void loadSettings();
    void saveSettings();
    UpdateRepository *updateRepositoryThread;
    Commit *commitThread;
    LoadPackage *loadPackageThread;
    QModelIndex sourcePkgIndex(QModelIndex index);
    TagsModel *tagsModel;
    CategoryModel *categoryModel;
    QSortFilterProxyModel *sortCategoryModel;
    QDataWidgetMapper *mapper;
    void runCommit();

public slots:
    void updateSlot();

private slots:
    void about();
    void pkgTagsFilter(QModelIndex tagIndex);
    void selectAll();
    void unSelectAll();
    void resetSelect();
    void pkgTextFilter(QString text);
    void chFilterColumn();
    void showSettings();
    void updateFinishedSlot();
    void updateRepositoryDataSlot();
    void updateRepositoryDataFinishedSlot();
    void commitSlot();
    void commitFinishedSlot();
    void refreshPkgInfo(QModelIndex pkgIndex);
    void resetQueue();
    void cleanCache();
    void setTagsModel();
    void showError(QString title, QString text, QString details);
    void openFile();
    void setRepFilter(int comboBoxIndex);
};

#endif // MAINWINDOW_H
