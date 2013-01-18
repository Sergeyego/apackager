#include "edtcategorydialog.h"
#include "ui_edtcategorydialog.h"

EdtCategoryDialog::EdtCategoryDialog(CategoryModel *catModel, int index, QStringList &tags, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EdtCategoryDialog)
{
    ui->setupUi(this);
    categoryModel=catModel;
    row=index;
    ui->lineEditName->setValidator(new QRegExpValidator(QRegExp("[A-Za-z _0-9-]*"), this));
    ui->lineEditName->setText(categoryModel->data(categoryModel->index(row,3)).toString());
    QStringList tagsCat=categoryModel->data(categoryModel->index(row,1)).toStringList();
    ui->listWidgetTags->addItems(tagsCat);
    for (int i=0; i<tags.size(); ++i){
        if (tagsCat.indexOf(tags.at(i))==-1)
            ui->listWidgetAvTags->addItem(tags[i]);
    }
    QStringList trCat=categoryModel->data(categoryModel->index(row,4)).toStringList();
    for (int i=0; i<trCat.size(); ++i){
        QListWidgetItem *item = new QListWidgetItem(trCat.at(i));
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
        ui->listWidgetTranslate->addItem(item);
    }
    QDir dir(categoryModel->getIconPrefix());
    QStringList filters;
    filters << "*.png";
    QStringList files = dir.entryList(filters);
    for (int i=0; i<files.size(); ++i){
        QString text=files.at(i);
        text.truncate(text.size()-4);
        ui->comboBoxIcon->addItem(QIcon(categoryModel->getIconPrefix()+"/"+files.at(i)),text);
    }
    QString icon=categoryModel->getIconName(categoryModel->index(row,0));
    ui->comboBoxIcon->setCurrentIndex(ui->comboBoxIcon->findText(icon));
    connect(ui->cmdAddtranslate,SIGNAL(clicked()),this,SLOT(addTranslation()));
    connect(categoryModel,SIGNAL(sigError(QString)),this,SLOT(errorBox(QString)));
}

EdtCategoryDialog::~EdtCategoryDialog()
{
    delete ui;
}

bool EdtCategoryDialog::save()
{
    QStringList tags;
    for (int i=0; i<ui->listWidgetTags->count(); ++i)
        tags.push_back(ui->listWidgetTags->item(i)->text());
    categoryModel->setData(categoryModel->index(row,1),tags,Qt::EditRole);
    QStringList translation;
    for (int i=0; i<ui->listWidgetTranslate->count(); ++i)
        translation.push_back(ui->listWidgetTranslate->item(i)->text());
    categoryModel->setData(categoryModel->index(row,4),translation,Qt::EditRole);
    categoryModel->setData(categoryModel->index(row,0),ui->comboBoxIcon->currentText(),Qt::DecorationRole);
    return categoryModel->setData(categoryModel->index(row,3),ui->lineEditName->text(),Qt::EditRole);
}

void EdtCategoryDialog::addTranslation()
{
    if (ui->lineEditAddTranslate->text().isEmpty()) return;
    QListWidgetItem *item = new QListWidgetItem(ui->lineEditAddTranslate->text());
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
    ui->listWidgetTranslate->addItem(item);
    ui->lineEditAddTranslate->clear();
    ui->listWidgetTranslate->scrollToBottom();
}

void EdtCategoryDialog::accept()
{
    if (save())
        QDialog::accept();
    else return;
}

void EdtCategoryDialog::errorBox(QString text)
{
    QMessageBox::critical(this,tr("Error"),text);
}
