#include "mpkgmodel.h"
#define ICON_PREFIX QString(":/icons/")
//QString(INSTALL_PREFIX) + QString("/share/mpkg/apackager/icons/")
//QString(":/icons/")

MpkgModel::MpkgModel(MpkgEngine *engine, QObject *parent) :
    QAbstractTableModel(parent)
{
    mpkg=engine;
    ccount=8;
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
        case 7:
            value=mpkg->cacheBase[index.row()].packageDistroVersion;
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
    QStringList pkgtags=this->sourceModel()->data(this->sourceModel()->index(source_row,3)).toStringList();
    QString rep=this->sourceModel()->data(this->sourceModel()->index(source_row,7)).toString();;

    bool isBlacklisted=false;
    int i=0;
    while(!isBlacklisted && i<blackList.size() && state==7){
        isBlacklisted=blackList.at(i)==name;
        ++i;
    }

    bool isTagFiltered(true);
    if (!tagFilterList.isEmpty()){
        for (int m=0; m<tagFilterList.size(); ++m)
            for (int n=0; n<pkgtags.size(); ++n)
                if (tagFilterList.at(m)==pkgtags.at(n)) isTagFiltered=false;
    } else isTagFiltered=false;

    bool isStateFiltered(true);
    if (!stateFilterList.isEmpty()){
        for(int k=0; k<stateFilterList.size(); ++k)
            if (state==stateFilterList.at(k).toInt()) isStateFiltered=false;
    } else isStateFiltered=false;

    bool isRepFiltered=repFilter.isNull()? false : !(rep==repFilter);

    return !(state==6 && hideDeprecated) && !(state==1 && hideInstalled) && !(state==3 && hideNotInstalled)
            && !(state==5 && hideInstalledDeprecated) && !(state==7 && hideUpdated) && !(isBlacklisted)
            && !(isTagFiltered) && !(isStateFiltered) && !(isRepFiltered);
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

void MpkgProxyModel::setFilter(QStringList taglist, QStringList statelist)
{
    tagFilterList=taglist;
    stateFilterList=statelist;
    reset();
}

void MpkgProxyModel::setRepositoryFilter(QString rep)
{
    repFilter=rep;
    reset();
}

TagsModel::TagsModel(MpkgEngine *engine, QObject *parent):QAbstractTableModel(parent)
{
    mpkg=engine;
    categoryData data;

    data.name=tr("All packages");
    data.tagList=QStringList();
    data.stateList=QStringList();
    bonus.push_back(data);

    data.name=tr("Updates");
    data.stateList<<QString::number(ICONSTATE_UPDATE);
    bonus.push_back(data);
    data.stateList.clear();

    data.name=tr("Installed");
    data.stateList<<QString::number(ICONSTATE_INSTALLED)<<QString::number(ICONSTATE_INSTALLED_DEPRECATED);
    bonus.push_back(data);
    data.stateList.clear();

    data.name=tr("Not installed");
    data.stateList<<QString::number(ICONSTATE_AVAILABLE)<<QString::number(ICONSTATE_AVAILABLE_DEPRECATED);
    bonus.push_back(data);
    data.stateList.clear();

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
    QStringList tag;
    if ((role==Qt::DisplayRole)||(role==Qt::EditRole)){
        int n=bonus.size();
        switch(index.column())
        {
            case 0:
                value=(index.row()<n)? bonus[index.row()].name : tags[index.row()-n];
                break;
            case 1:
                if (index.row()<n) {
                    value=bonus[index.row()].tagList;
                }else {
                    tag<<tags[index.row()-n];
                    value=tag;
                }
                break;
            case 2:
            value=(index.row()<n)? bonus[index.row()].stateList : QStringList();
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
    addCategory(tr("All packages"),"applications-system",QString(),QString());
    addCategory(tr("Updates"),"update",QString(),QString::number(ICONSTATE_UPDATE));
    addCategory(tr("Installed"),"installed",QString(),QString::number(ICONSTATE_INSTALLED)+","+QString::number(ICONSTATE_INSTALLED_DEPRECATED));
    addCategory(tr("Not installed"),"available",QString(),QString::number(ICONSTATE_AVAILABLE)+","+QString::number(ICONSTATE_AVAILABLE_DEPRECATED));
    addCategory(tr("Office"),"office","app-office",QString());
    addCategory(tr("Emulations"),"system","app-emulation",QString());
    addCategory(tr("Compat 32"),"system","compat32,x86",QString());
    addCategory(tr("Console applications"),"terminal","console",QString());
    addCategory(tr("Development"),"development","develop",QString());
    addCategory(tr("Drivers"),"drivers","drivers",QString());
    addCategory(tr("Games"),"games","games",QString());
    addCategory(tr("Gnome"),"logo-gnome","gnome",QString());
    addCategory(tr("KDE"),"logo-kde","kde4,kdei,kdel10n",QString());
    addCategory(tr("LXDE"),"logo-lxde","lxde",QString());
    addCategory(tr("Mail"),"mail","mail-client,mail-mta",QString());
    addCategory(tr("Fonts"),"fonts","media-fonts",QString());
    addCategory(tr("Graphics"),"graphics","media-gfx",QString());
    addCategory(tr("Sound"),"audio","media-sound",QString());
    addCategory(tr("Network"),"internet","network",QString());
    addCategory(tr("Proprietary"),"proprietary","proprietary",QString());
    addCategory(tr("Science"),"science","school",QString());
    addCategory(tr("Server"),"server","server",QString());
    addCategory(tr("Kernel"),"logo-linux","sys-kernel",QString());
    addCategory(tr("System"),"system","sys-apps,sys-auth,sys-base,sys-boot,sys-devel,sys-fs,sys-kernel,sys-libs,sys-pkgtools,sys-power,sys-process",QString());
    addCategory(tr("Themes"),"theme","themes",QString());
    addCategory(tr("Utils"),"system","utils",QString());
    addCategory(tr("XFCE"),"logo-xfce","xfce",QString());
}

void CategoryModel::addCategory(QString cat_name, QString cat_ico, QString cat_tags, QString cat_state)
{
    categoryData data;
    data.name=cat_name;
    data.icon=QIcon(ICON_PREFIX+cat_ico+".png");
    data.tagList=cat_tags.size()? cat_tags.split(",") : QStringList();
    data.stateList=cat_state.size()? cat_state.split(",") : QStringList();
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
                value=catData[index.row()].tagList;
                break;
            case 2:
                value=catData[index.row()].stateList;
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
