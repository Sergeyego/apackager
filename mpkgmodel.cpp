#include "mpkgmodel.h"
#define ICON_PREFIX QString(INSTALL_PREFIX) + QString("/share/mpkg/apackager/icons/")
//QString(":/icons/")

MpkgModel::MpkgModel(MpkgEngine *engine, QObject *parent) :
    QAbstractTableModel(parent)
{
    mpkg=engine;
    ccount=7;
    loadPixmapList();
    header<<tr("Name")<<tr("Version")<<tr("Short description");
    connect(engine,SIGNAL(updateFinished()),this,SLOT(refresh()));
}

Qt::ItemFlags MpkgModel::flags(const QModelIndex &index) const
{
    return (index.isValid())?
                Qt::ItemIsSelectable |Qt::ItemIsUserCheckable | Qt::ItemIsEnabled : Qt::ItemIsEnabled;
}

int MpkgModel::rowCount(const QModelIndex &parent) const
{
    return mpkg->cacheBase.size();
}

int MpkgModel::columnCount(const QModelIndex &parent) const
{
    return ccount;
}

QVariant MpkgModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();
    if (role==Qt::CheckStateRole && index.column()==0){
        return (mpkg->pkgIsInstalled(index.row())^chk[index.row()])? Qt::Checked : Qt::Unchecked;
    }
    if (role==Qt::DecorationRole && index.column()==0){
        int state=mpkg->cacheBase[index.row()].state;
        bool isInst=mpkg->pkgIsInstalled(index.row());
        if (isInst && chk[index.row()]) state=4;
        if (!isInst && chk[index.row()]) state=2;
        return QIcon(pixmapList[state]);
    }
    QVariant value;
    if ((role==Qt::DisplayRole)||(role==Qt::EditRole)){
        switch(index.column())
        {
        case 0:
            value=mpkg->cacheBase[index.row()].name;
            break;
        case 1:
            value=mpkg->cacheBase[index.row()].version;
            break;
        case 2:
            value=mpkg->cacheBase[index.row()].short_discription;
            break;
        case 3:
            value=mpkg->cacheBase[index.row()].tags;
            break;
        case 4:
            value=mpkg->cacheBase[index.row()].state;
            break;
        case 5:
            value=mpkg->cacheBase[index.row()].info;
            break;
        case 6:
            value=mpkg->cacheBase[index.row()].dependency;
            break;
        default:
            value=QVariant();
            break;
        }
    }
    return value;
}

QVariant MpkgModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    return (section< header.size()&& orientation==Qt::Horizontal && role == Qt::DisplayRole)?
                header[section] : QAbstractItemModel::headerData(section,orientation,role);
}

bool MpkgModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.isValid() && role==Qt::CheckStateRole && index.column()==0) {
        chk[index.row()]=value.toBool()^mpkg->pkgIsInstalled(index.row());
        emit dataChanged(index,index);
        return true;
    }
    return false;
}

QStringList MpkgModel::headerList()
{
    return header;
}

bool MpkgModel::refreshQueue()
{
    QVector<int> tmpInstInd, tmpRemInd;
    for (unsigned int i=0; i<chk.size(); ++i){
        if (chk[i]) mpkg->pkgIsInstalled(i)? tmpRemInd.push_back(i): tmpInstInd.push_back(i);
    }
    return mpkg->createQueue(tmpInstInd,tmpRemInd);
}

bool MpkgModel::actionsIsEmpty()
{
    for (unsigned int i=0; i<chk.size(); ++i){
        if (chk[i]) return false;
    }
    return true;
}

void MpkgModel::loadPixmapList()
{
    pixmapList.push_back(QPixmap(ICON_PREFIX+"unknown.png"));
    pixmapList.push_back(QPixmap(ICON_PREFIX+"installed.png"));
    pixmapList.push_back(QPixmap(ICON_PREFIX+"install.png"));
    pixmapList.push_back(QPixmap(ICON_PREFIX+"available.png"));
    pixmapList.push_back(QPixmap(ICON_PREFIX+"remove.png"));
    pixmapList.push_back(QPixmap(ICON_PREFIX+"deprecated_installed.png"));
    pixmapList.push_back(QPixmap(ICON_PREFIX+"deprecated_available.png"));
    pixmapList.push_back(QPixmap(ICON_PREFIX+"update.png"));
}

void MpkgModel::refresh()
{
    mpkg->resetQueue();
    chk.resize(mpkg->cacheBase.size());
    chk.fill(false);
    reset();
}

MpkgProxyModel::MpkgProxyModel(QObject *parent):QSortFilterProxyModel(parent)
{
    hideDeprecated=true;
    hideInstalled=false;
    hideNotInstalled=false;
    hideInstalledDeprecated=false;
    hideUpdated=false;
}

bool MpkgProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    int state=this->sourceModel()->data(this->sourceModel()->index(source_row,4)).toInt();
    QString name=this->sourceModel()->data(this->sourceModel()->index(source_row,0)).toString();
    bool b=false;
    int i=0;
    while(!b && i<blackList.size() && state==7){
        b=blackList.at(i)==name;
        ++i;
    }
    return !(state==6 && hideDeprecated) && !(state==1 && hideInstalled) && !(state==3 && hideNotInstalled)
            && !(state==5 && hideInstalledDeprecated) && !(state==7 && hideUpdated) && !(b);
}

void MpkgProxyModel::setHideDeprecated(bool value)
{
    hideDeprecated=value;
    reset();
}

void MpkgProxyModel::setHideInstalled(bool value)
{
    hideInstalled=value;
    reset();
}

void MpkgProxyModel::setHideNotInstalled(bool value)
{
    hideNotInstalled=value;
    reset();
}

void MpkgProxyModel::setHideInstalledDeprecated(bool value)
{
    hideInstalledDeprecated=value;
    reset();
}

void MpkgProxyModel::setHideUpdated(bool value)
{
    hideUpdated=value;
    reset();
}

void MpkgProxyModel::setBlacklist(QStringList list)
{
    blackList=list;
    reset();
}

TagsModel::TagsModel(MpkgEngine *engine, QObject *parent):QAbstractTableModel(parent)
{
    mpkg=engine;
    categoryData data;
    data.filterKeyColumn=4;
    data.name=tr("All packages");
    data.regExp="";
    bonus.push_back(data);
    data.name=tr("Updates");
    data.regExp=QString::number(ICONSTATE_UPDATE);
    bonus.push_back(data);
    data.name=tr("Installed");
    data.regExp=QString("[")+QString::number(ICONSTATE_INSTALLED)+QString::number(ICONSTATE_INSTALLED_DEPRECATED)+QString("]");
    bonus.push_back(data);
    data.name=tr("Not installed");
    data.regExp=QString("[")+QString::number(ICONSTATE_AVAILABLE)+QString::number(ICONSTATE_AVAILABLE_DEPRECATED)+QString("]");
    bonus.push_back(data);
    connect(mpkg,SIGNAL(updateFinished()),this,SLOT(refresh()));
}

Qt::ItemFlags TagsModel::flags(const QModelIndex &index) const
{
    return (index.isValid())? Qt::ItemIsSelectable | Qt::ItemIsEnabled : Qt::ItemIsEnabled;
}

int TagsModel::rowCount(const QModelIndex &parent) const
{
    return tags.size()+bonus.size();
}

int TagsModel::columnCount(const QModelIndex &parent) const
{
    return 3;
}

QVariant TagsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();
    QVariant value;
    if ((role==Qt::DisplayRole)||(role==Qt::EditRole)){
        int n=bonus.size();
        switch(index.column())
        {
            case 0:
                value=(index.row()<n)? bonus[index.row()].name : tags[index.row()-n];
                break;
            case 1:
                value=(index.row()<n)? bonus[index.row()].regExp : QString(":"+tags[index.row()-n]+":");
                break;
            case 2:
                value=(index.row()<n)? bonus[index.row()].filterKeyColumn : 3;
                break;
            default:
                value=QVariant();
                break;
        }
    }
    return value;
}

void TagsModel::refresh()
{
    tags=mpkg->availableTags();
    reset();
}

CategoryModel::CategoryModel(QObject *parent):QAbstractTableModel(parent)
{
    categoryData data;
    data.filterKeyColumn=4;
    data.name=tr("All packages");
    data.icon=QIcon(ICON_PREFIX+"applications-system.png");
    data.regExp="";
    catData.push_back(data);
    data.name=tr("Updates");
    data.icon=QIcon(ICON_PREFIX+"update.png");
    data.regExp=QString::number(ICONSTATE_UPDATE);
    catData.push_back(data);
    data.name=tr("Installed");
    data.icon=QIcon(ICON_PREFIX+"installed.png");
    data.regExp=QString("[")+QString::number(ICONSTATE_INSTALLED)+QString::number(ICONSTATE_INSTALLED_DEPRECATED)+QString("]");
    catData.push_back(data);
    data.name=tr("Not installed");
    data.icon=QIcon(ICON_PREFIX+"available.png");
    data.regExp=QString("[")+QString::number(ICONSTATE_AVAILABLE)+QString::number(ICONSTATE_AVAILABLE_DEPRECATED)+QString("]");
    catData.push_back(data);
    data.filterKeyColumn=3;
    data.name=tr("Office");
    data.icon=QIcon(ICON_PREFIX+"office.png");
    data.regExp=":app-office:";
    catData.push_back(data);
    data.name=tr("Emulations");
    data.icon=QIcon(ICON_PREFIX+"system.png");
    data.regExp=":app-emulation:";
    catData.push_back(data);
    data.name=tr("Compat 32");
    data.icon=QIcon(ICON_PREFIX+"system.png");
    data.regExp=":compat32:";
    catData.push_back(data);
    data.name=tr("Console applications");
    data.icon=QIcon(ICON_PREFIX+"terminal.png");
    data.regExp=":console:";
    catData.push_back(data);
    data.name=tr("Development");
    data.icon=QIcon(ICON_PREFIX+"development.png");
    data.regExp=":dev";
    catData.push_back(data);
    data.name=tr("Drivers");
    data.icon=QIcon(ICON_PREFIX+"drivers.png");
    data.regExp=":drivers:";
    catData.push_back(data);
    data.name=tr("Games");
    data.icon=QIcon(ICON_PREFIX+"games.png");
    data.regExp=":games:";
    catData.push_back(data);
    data.name=tr("Gnome");
    data.icon=QIcon(ICON_PREFIX+"logo-gnome.png");
    data.regExp=":gnome:";
    catData.push_back(data);
    data.name=tr("KDE");
    data.icon=QIcon(ICON_PREFIX+"logo-kde.png");
    data.regExp=":kde";
    catData.push_back(data);
    data.name=tr("LXDE");
    data.icon=QIcon(ICON_PREFIX+"logo-lxde.png");
    data.regExp=":lxde:";
    catData.push_back(data);
    data.name=tr("Mail");
    data.icon=QIcon(ICON_PREFIX+"mail.png");
    data.regExp=":mail";
    catData.push_back(data);
    data.name=tr("Fonts");
    data.icon=QIcon(ICON_PREFIX+"fonts.png");
    data.regExp="fonts";
    catData.push_back(data);
    data.name=tr("Graphics");
    data.icon=QIcon(ICON_PREFIX+"graphics.png");
    data.regExp=":media-gfx:";
    catData.push_back(data);
    data.name=tr("Sound");
    data.icon=QIcon(ICON_PREFIX+"audio.png");
    data.regExp="sound";
    catData.push_back(data);
    data.name=tr("Network");
    data.icon=QIcon(ICON_PREFIX+"internet.png");
    data.regExp=":network:";
    catData.push_back(data);
    data.name=tr("Proprietary");
    data.icon=QIcon(ICON_PREFIX+"proprietary.png");
    data.regExp=":proprietary:";
    catData.push_back(data);
    data.name=tr("Science");
    data.icon=QIcon(ICON_PREFIX+"science.png");
    data.regExp=":school:";
    catData.push_back(data);
    data.name=tr("Server");
    data.icon=QIcon(ICON_PREFIX+"server.png");
    data.regExp=":server:";
    catData.push_back(data);
    data.name=tr("Kernel");
    data.icon=QIcon(ICON_PREFIX+"logo-linux.png");
    data.regExp="kernel";
    catData.push_back(data);
    data.name=tr("System");
    data.icon=QIcon(ICON_PREFIX+"system.png");
    data.regExp="sys-";
    catData.push_back(data);
    data.name=tr("Themes");
    data.icon=QIcon(ICON_PREFIX+"theme.png");
    data.regExp=":themes:";
    catData.push_back(data);
    data.name=tr("Utils");
    data.icon=QIcon(ICON_PREFIX+"system.png");
    data.regExp="utils";
    catData.push_back(data);
    data.name=tr("XFCE");
    data.icon=QIcon(ICON_PREFIX+"logo-xfce.png");
    data.regExp="xfce";
    catData.push_back(data);
}

Qt::ItemFlags CategoryModel::flags(const QModelIndex &index) const
{
    return (index.isValid())? Qt::ItemIsSelectable | Qt::ItemIsEnabled : Qt::ItemIsEnabled;
}

int CategoryModel::rowCount(const QModelIndex &parent) const
{
    return catData.size();
}

int CategoryModel::columnCount(const QModelIndex &parent) const
{
    return 3;
}

QVariant CategoryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();
    if (role==Qt::DecorationRole && index.column()==0){
        return catData[index.row()].icon;
    }
    QVariant value;
    if ((role==Qt::DisplayRole)||(role==Qt::EditRole)){
        switch(index.column())
        {
            case 0:
                value=catData[index.row()].name;
                break;
            case 1:
                value=catData[index.row()].regExp;
                break;
            case 2:
                value=catData[index.row()].filterKeyColumn;
                break;
            default:
                value=QVariant();
                break;
        }
    }
    return value;
}

MpkgSearchModel::MpkgSearchModel(QObject *parent):QSortFilterProxyModel(parent)
{
}

bool MpkgSearchModel::filterAcceptsColumn(int source_column, const QModelIndex &source_parent) const
{
    return !(source_column>2);
}
