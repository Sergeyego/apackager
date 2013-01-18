#ifndef MPKGSETTINGS_H
#define MPKGSETTINGS_H

#include <QtGui>
#include "mpkgengine.h"
#include "mpkgmodel.h"
#include "edtcategorydialog.h"

namespace Ui {
class MpkgSettings;
}

class MpkgSettings : public QDialog
{
    Q_OBJECT
    
public:
    explicit MpkgSettings(MpkgEngine *engine, CategoryModel *model, QWidget *parent = 0);
    ~MpkgSettings();
    void keyPressEvent(QKeyEvent *p);
public slots:
    void save();
private:
    Ui::MpkgSettings *ui;
    MpkgEngine *mpkg;
    CategoryModel *categoryModel;
    ProxyCategoryModel *proxyCategoryModel;
    void load();
    QString toStr(bool val);
private slots:
    void addRepository();
    void refreshRepView();
    void addRemoveProtected();
    void addUpdateBlacklist();
    void addCategory();
    void removeCategory();
    void edtCategory();
};

class SettingsListWidget : public QListWidget
{
    Q_OBJECT
public:
    SettingsListWidget(QWidget *parent = 0);
    void keyPressEvent(QKeyEvent *event);
    void contextMenuEvent(QContextMenuEvent *event);
private:
    QAction *removeAction;
private slots:
    void removeCurrentItem();
};

#endif // MPKGSETTINGS_H
