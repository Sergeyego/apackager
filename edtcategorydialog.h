#ifndef EDTCATEGORYDIALOG_H
#define EDTCATEGORYDIALOG_H

#include <QtGui>
#include "mpkgmodel.h"
#include <QDataWidgetMapper>
#include <QDir>

namespace Ui {
class EdtCategoryDialog;
}

class EdtCategoryDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit EdtCategoryDialog(CategoryModel *catModel, int index, QStringList &tags, QWidget *parent = 0);
    ~EdtCategoryDialog();
    bool save();
private:
    Ui::EdtCategoryDialog *ui;
    CategoryModel *categoryModel;
    int row;
private slots:
    void addTranslation();
    void accept();
    void errorBox(QString text);
};

#endif // EDTCATEGORYDIALOG_H
