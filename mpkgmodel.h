#ifndef MPKGMODEL_H
#define MPKGMODEL_H

#include <QAbstractTableModel>
#include <QPixmap>
#include "mpkgengine.h"
#include <QIcon>
#include <QDebug>
#include <QSortFilterProxyModel>

class MpkgModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit MpkgModel(MpkgEngine *engine,  QObject *parent = 0);
    Qt::ItemFlags flags(const QModelIndex &index) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);
    QStringList headerList();
    bool refreshQueue();
    bool actionsIsEmpty();

private:
    int ccount;
    MpkgEngine *mpkg;
    QVector<bool> chk;
    QVector<QPixmap> pixmapList;
    void loadPixmapList();
    QStringList header;

signals:
    
public slots:
    void refresh();    
};

class MpkgProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    MpkgProxyModel(QObject *parent = 0);
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;
private:
    bool hideDeprecated;
    bool hideInstalled;
    bool hideNotInstalled;
    bool hideInstalledDeprecated;
    bool hideUpdated;
    QStringList blackList;
    QStringList tagFilterList;
    QStringList stateFilterList;
    QString repFilter;
public slots:
    void setHideDeprecated(bool value);
    void setHideInstalled(bool value);
    void setHideNotInstalled(bool value);
    void setHideInstalledDeprecated(bool value);
    void setHideUpdated(bool value);
    void setBlacklist(QStringList list);
    void setFilter(QStringList taglist, QStringList statelist);
    void setRepositoryFilter(QString rep);
};

class MpkgSearchModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    MpkgSearchModel(QObject *parent = 0);
    bool filterAcceptsColumn(int source_column, const QModelIndex &source_parent) const;
};

typedef struct{
    QString name;
    QIcon icon;
    QStringList tagList;
    QStringList stateList;
} categoryData;

class TagsModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    TagsModel(MpkgEngine *engine,  QObject *parent = 0);
    Qt::ItemFlags flags(const QModelIndex &index) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
private:
    MpkgEngine *mpkg;
    QVector<categoryData> bonus;
    QStringList tags;
public slots:
    void refresh();
};

class CategoryModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    CategoryModel(QObject *parent = 0);
    Qt::ItemFlags flags(const QModelIndex &index) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
private:
    void addCategory(QString cat_name, QString cat_ico, QString cat_tags, QString cat_state);
    QVector<categoryData> catData;
};

#endif // MPKGMODEL_H
