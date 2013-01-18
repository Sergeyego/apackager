#include "mpkgmodel.h"
//#define ICON_PREFIX QString(":/icons/")
#define ICON_PREFIX QString(INSTALL_PREFIX) + QString("/share/mpkg/apackager/icons/")

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
            value=mpkg->getPkgInfo(index.row());
            break;
        case 6:
            value=mpkg->getPkgDependencies(index.row());
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
    addBonus(tr("All packages"),QString());
    addBonus(tr("Updates"),QString::number(ICONSTATE_UPDATE));
    addBonus(tr("Installed"),QString::number(ICONSTATE_INSTALLED)+","+QString::number(ICONSTATE_INSTALLED_DEPRECATED));
    addBonus(tr("Not installed"),QString::number(ICONSTATE_AVAILABLE)+","+QString::number(ICONSTATE_AVAILABLE_DEPRECATED));

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
            value=(index.row()<n)? bonus[index.row()].name[0] : tags[index.row()-n];
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

void TagsModel::addBonus(QString name, QString statelist, QString taglist)
{
    categoryData data;
    data.name.push_back(name);
    data.tagList=taglist.size()? taglist.split(",") : QStringList();
    data.stateList=statelist.size()? statelist.split(",") : QStringList();
    bonus.push_back(data);
}

void TagsModel::refresh()
{
    tags=mpkg->availableTags();
    reset();
}

CategoryModel::CategoryModel(QObject *parent):QAbstractTableModel(parent)
{
    refresh();
}

void CategoryModel::addCategory(QString cat_name, QString cat_name_local, QString cat_ico, QString cat_tags, QString cat_state)
{
    QMap <QString, QString> map;
    map["name[en]"]=cat_name;
    map["name"+getLocale()]=cat_name_local;
    map["tags"]=cat_tags;
    map["icon"]=cat_ico;
    map["state"]=cat_state;
    catData.push_back(map);
}

Qt::ItemFlags CategoryModel::flags(const QModelIndex &index) const
{
    return (index.column()==1 || index.column()==3 || index.column()==4)?
                (Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsSelectable) : (Qt::ItemIsEnabled | Qt::ItemIsSelectable);
}

int CategoryModel::rowCount(const QModelIndex &parent) const
{
    return catData.size();
}

int CategoryModel::columnCount(const QModelIndex &parent) const
{
    return 5;
}

QString CategoryModel::getLocale() const
{
    QLocale lc;
    QString namelc=lc.name();
    namelc.truncate(namelc.indexOf("_"));
    namelc.push_front("[");
    namelc.push_back("]");
    return namelc;
}

QVariant CategoryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();
    if (role==Qt::DecorationRole && (index.column()==0 || index.column()==3)){
        return QIcon(ICON_PREFIX+catData[index.row()].value("icon")+".png");
    }
    QVariant value;
    if ((role==Qt::DisplayRole)||(role==Qt::EditRole)){
        switch(index.column())
        {
            case 0:
            {
                QString localName=catData[index.row()].value("name"+getLocale());
                value=localName.isNull()? catData[index.row()].value("name[en]") : localName;
                break;
            }
            case 1:
            {
                QString tags=catData[index.row()].value("tags");
                value=tags.size()? tags.split(",") : QStringList();
                break;
            }
            case 2:
            {
                QString state=catData[index.row()].value("state");
                value=state.size()? state.split(",") : QStringList();
                break;
            }
            case 3:
            {
                value=catData[index.row()].value("name[en]");
                break;
            }
            case 4:
            {
                QStringList translationList;
                QMap<QString, QString> map=catData[index.row()];
                QMap<QString, QString>::const_iterator i = map.constBegin();
                while (i != map.constEnd()) {
                    QString key=i.key();
                    if (key.contains(QRegExp("name\\[[^\\]]*\\]$")) && key!="name[en]"){
                        translationList.push_back(key.replace("name","")+i.value());
                    }
                    ++i;
                }
                value=translationList;
                break;
            }
            default:
                value=QVariant();
                break;
        }
    }
    return value;
}

bool CategoryModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role==Qt::DecorationRole && index.column()==0){
        catData[index.row()]["icon"]=value.toString();
    }
    bool result(true);
    if (role==Qt::EditRole){
        switch (index.column()){
            case 1:
            {
                QString str;
                QStringList tags=value.toStringList();
                for (int i=0; i<tags.size(); ++i) str.push_back(tags.at(i)+",");
                str.truncate(str.size()-1);
                catData[index.row()]["tags"]=str;
                break;
            }
            case 3:
            {
                bool findName(false);
                int i(0);
                while (i<catData.size()&& !findName){
                    findName=(catData.at(i).value("name[en]")==value.toString())&& i!=index.row();
                    ++i;
                }
                if (!findName){
                    catData[index.row()]["name[en]"]=value.toString();
                } else {
                    result=false;
                    emit sigError(tr("Category with the same name already exists"));
                }
                break;
            }
            case 4:
            {
                QStringList str=value.toStringList();
                QRegExp reg("^[ ]*(\\[[^\\]]+\\])([^\\]]+)$");
                QMap<QString, QString>::const_iterator iter = catData[index.row()].constBegin();
                while (iter != catData[index.row()].constEnd()) {
                    QString key=iter.key();
                    if (key.contains(QRegExp("name\\[[^\\]]*\\]$")) && key!="name[en]"){
                        catData[index.row()].remove(key);
                    }
                    ++iter;
                }
                for (int i=0; i<str.size(); ++i){
                    if (reg.indexIn(str.at(i))!=-1){
                        QString lang=reg.cap(1);
                        QString val=reg.cap(2);
                        //qDebug()<<"LANG= "<<lang<<"VAL= "<<val;
                        if (lang!="[en]") catData[index.row()]["name"+lang]=val;
                    }
                }
                break;
            }
            default:
                return false;
                break;
        }
    } else
        return false;
    emit dataChanged(index,index);
    return result;
}

void CategoryModel::insertRow()
{
    int n(0);
    int i(0);
    bool findName(false);
    do {
        n++;
        i=0;
        findName=false;
        while (i<catData.size()&& !findName){
            findName=(catData.at(i).value("name[en]")=="New category-"+QString::number(n));
            ++i;
        }
    } while(findName);
    addCategory("New category-"+QString::number(n),tr("New category")+"-"+QString::number(n),"applications-system",QString(),QString());
    reset();
}

bool CategoryModel::removeRow(int row, const QModelIndex &parent)
{
    if (row<0 || row>=catData.size()) return false;
    catData.remove(row,1);
    reset();
    return true;
}

void CategoryModel::refresh()
{
    catData.clear();
    addCategory(" All packages"," "+tr("All packages"),"applications-system",QString(),QString());
    addCategory(" Updates"," "+tr("Updates"),"update",QString(),QString::number(ICONSTATE_UPDATE));
    addCategory(" Installed"," "+tr("Installed"),"installed",QString(),QString::number(ICONSTATE_INSTALLED)+","+QString::number(ICONSTATE_INSTALLED_DEPRECATED));
    addCategory(" Not installed"," "+tr("Not installed"),"available",QString(),QString::number(ICONSTATE_AVAILABLE)+","+QString::number(ICONSTATE_AVAILABLE_DEPRECATED));
    QDir dir("/etc/apackager/categories.d");
    QMap <QString, QString> map;
    QStringList filters;
    filters << "*.conf";
    QStringList files = dir.entryList(filters);
    for (int k=0; k<files.size(); ++k){
        map.clear();
        QFile file(dir.path()+"/"+files.at(k));
        file.open(QIODevice::ReadOnly | QIODevice::Text);
        QTextStream in(&file);
        in.setCodec(QTextCodec::codecForName("UTF8"));
        while(!in.atEnd()){
            QString str=in.readLine();
            QRegExp reg("^[ ]*([^= ]*)[ ]*=([^=]*)$");
            if (reg.indexIn(str)!=-1){
                QString param=reg.cap(1);
                QString val=reg.cap(2);
                param=param.toLower();
                val.remove(QRegExp("^[ ]*|[ ]$"));
                if (param=="tags") val.replace(" ","");
                map[param]=val;
                //qDebug()<<"param: "<<param<<" value: "<<val;
            }
        }
        file.close();
        catData.push_back(map);
    }
    reset();
}

void CategoryModel::commit()
{
    QDir dir("/etc/apackager/categories.d");
    if (!dir.exists()) dir.mkdir(dir.path());
    QStringList filters;
    filters << "*.conf";
    QStringList files = dir.entryList(filters);
    for (int k=0; k<files.size(); ++k)
        dir.remove(files.at(k));
    for (int i=4; i<rowCount(); ++i){
        QString catName=catData[i].value("name[en]").toLower();
        catName.replace(" ","_");
        QFile file(dir.path()+"/"+catName+".conf");
        file.open(QIODevice::WriteOnly | QIODevice::Text);
        QTextStream out(&file);
        out.setCodec(QTextCodec::codecForName("UTF8"));
        QMap<QString, QString>::const_iterator iter = catData.at(i).constBegin();
        while (iter != catData.at(i).constEnd()) {
            QString key=iter.key();
            if (key!="state"){
                if (key.size()) key=key.toUpper();
                out<<key<<" = "<<iter.value()<<"\n";
            }
            ++iter;
        }
        file.close();
    }
    refresh();
}

QString CategoryModel::getIconPrefix()
{
    return ICON_PREFIX;
}

QString CategoryModel::getIconName(const QModelIndex &index)
{
    return catData[index.row()].value("icon");
}

MpkgSearchModel::MpkgSearchModel(QObject *parent):QSortFilterProxyModel(parent)
{
}

bool MpkgSearchModel::filterAcceptsColumn(int source_column, const QModelIndex &source_parent) const
{
    return !(source_column>2);
}

ProxyCategoryModel::ProxyCategoryModel(QObject *parent):QSortFilterProxyModel(parent)
{
}

bool ProxyCategoryModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    return !(source_row<4);
}
