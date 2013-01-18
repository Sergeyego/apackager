#include "mpkgengine.h"

MpkgEngine::MpkgEngine(QObject *parent) :
    QObject(parent)
{
    core = new mpkg;
    pData.registerEventHandler(&MpkgHandler::updateProgressData);
    mpkgErrorHandler.registerErrorHandler(&MpkgHandler::qtErrorHandler);
}

MpkgEngine::~MpkgEngine()
{
    delete core;
    pData.unregisterEventHandler();
    mpkgErrorHandler.unregisterErrorHandler();
    unlockDatabase();
}

void MpkgEngine::updatePkgList()
{
    qDebug()<<"start update";
    SQLRecord sqlSearch;
    core->get_packagelist(sqlSearch, &packageList);
    packageList.initVersioning();
    int n=packageList.size();
    cacheBase.resize(n);
    mpkgPackage pkg;
    vector<string> tags;
    int lastIndex, installedIndex;
    int state;
    packageDistroVersionList.clear();
    for (unsigned int i=0; i< n; ++i){
        const PACKAGE *p=&packageList.at(i);
        pkg.name=QString::fromStdString(p->get_name());
        pkg.version=QString::fromStdString(p->get_fullversion());
        pkg.short_discription=QString::fromStdString(p->get_short_description());
        pkg.tags.clear();
        tags = p->get_tags();
        for (vector<string>::iterator it=tags.begin(); it!=tags.end(); ++it) {
            pkg.tags.push_back(QString::fromStdString(*it));
        }

        lastIndex=packageList.getMaxVersionNumber(p->get_name());
        installedIndex=packageList.getInstalledVersionNumber(p->get_name());
        if (p->installed()) {
            state = ((i==lastIndex)&&(i==installedIndex))? ICONSTATE_INSTALLED : ICONSTATE_INSTALLED_DEPRECATED;
        } else if (p->available()) {
            if (installedIndex!=-1) {
                state = (i==lastIndex)? ICONSTATE_UPDATE : ICONSTATE_AVAILABLE_DEPRECATED;
            } else {
                state = (i==lastIndex)? ICONSTATE_AVAILABLE : ICONSTATE_AVAILABLE_DEPRECATED;
            }
        } else {
            state = ICONSTATE_UNKNOWN;
        }
        pkg.state=state;

        pkg.packageDistroVersion=QString::fromStdString(p->package_distro_version);
        if (packageDistroVersionList.indexOf(pkg.packageDistroVersion)==-1) packageDistroVersionList.push_back(pkg.packageDistroVersion);

        cacheBase[i]=pkg;
    }
    vector<string> available_tags;
    core->get_available_tags(&available_tags);
    tagsList.clear();
    tagsList=toQStringList(available_tags);
    tagsList.sort();
    packageDistroVersionList.sort();
    qDebug()<<"update finished";
    emit updateFinished();
}

QString MpkgEngine::getPkgInfo(const int &index) const
{
    QString info;
    const PACKAGE *p=&packageList[index];
    vector<string> tags=p->get_tags();
    QString tagList, linkList;
    vector<LOCATION> loc=p->get_locations();
    for (vector<string>::iterator it=tags.begin(); it!=tags.end(); ++it) {
        tagList.push_back(QString::fromStdString((*it)+" "));
    }
    if (!tagList.isEmpty()) {
        tagList.truncate(tagList.size()-1);
        tagList = "<hr><big><b>" + tr("Tags") + ":</b></big> " + tagList;
    }
    for (vector<LOCATION>::iterator it=loc.begin(); it!=loc.end(); ++it) {
        linkList.push_back("<a href=\"" + QString::fromStdString((*it).get_full_url() + p->get_filename()) + "\">" + QString::fromStdString(p->get_filename()) + "</a><br>");
    }
    QString description=(p->get_description().size())? QString::fromStdString(p->get_description()) :
                                                       QString::fromStdString(p->get_short_description());
    info = ("<b><big>" + QString::fromStdString(p->get_name()) + " " + QString::fromStdString(p->get_fullversion()) + "</b></big><hr>" + description + \
            "<hr><b>" + tr("Package size: ") + "</b>" + QString::fromStdString(humanizeSize(p->get_compressed_size())) + \
            "<br><b>" + tr("Installed size: ") + "</b>" + QString::fromStdString(humanizeSize(p->get_installed_size())) + \
            "<br><b>" + tr("Distrib: ") + "</b>" + QString::fromStdString(p->package_distro_version) + \
            "<br><b>" + tr("Maintainer: ") + "</b>" + QString::fromStdString(p->get_packager()) + \
            "<br><b>" + tr("File name: ") + "</b>" + QString::fromStdString(p->get_filename()) + \
            "<br><b>MD5: </b>" + QString::fromStdString(p->get_md5()) +
            tagList + "<hr><big><b>" + tr("Download links: ") + "</b></big><br>" + linkList);
    return info;
}

QString MpkgEngine::getPkgDependencies(const int &index) const
{
    vector<DEPENDENCY> deps;
    deps=packageList.at(index).get_dependencies();
    QString depList;
    for (vector<DEPENDENCY>::iterator it=deps.begin(); it!=deps.end(); ++it) {
        depList.push_back(QString::fromStdString((*it).getDepInfo())+"<br>");
    }
    return depList;
}

bool MpkgEngine::pkgIsInstalled(const int &index)
{
    return (cacheBase[index].state==ICONSTATE_INSTALLED) || (cacheBase[index].state==ICONSTATE_INSTALLED_DEPRECATED);
}

QStringList MpkgEngine::availableTags()
{
    return tagsList;
}

void MpkgEngine::resetQueue()
{
    core->clean_queue();
    core->DepTracker->reset();
}

void MpkgEngine::cleanCache()
{
    core->clean_cache();
}

bool MpkgEngine::createQueue(const QVector <int> &pkgInst, const QVector <int> &pkgRem)
{
    PACKAGE_LIST tmpIQueue, tmpRQueue;

    for (QVector<int>::const_iterator it=pkgInst.constBegin(); it!=pkgInst.constEnd(); ++it){
        tmpIQueue.add(packageList.at(*it));
    }

    for (QVector<int>::const_iterator it=pkgRem.constBegin(); it!=pkgRem.constEnd(); ++it){
        tmpRQueue.add(packageList.at(*it));
    }

    _abortActions=false;
    resetQueue();
    core->install(&tmpIQueue);
    core->uninstall(&tmpRQueue);
    return renderDepTrackerData();
}

bool MpkgEngine::createQueue(const QStringList &fname)
{
    vector<string> pkg, errors;
    QString errText;
    _abortActions=false;
    resetQueue();
    fromQStringList(fname,pkg);
    core->install(pkg, NULL, NULL, &errors);
    if (errors.size()) {
        for (int i=0; i<errors.size(); ++i) errText.push_back(QString::fromStdString(errors[i])+" ");
        emit sigError(tr("Error"),errText,"");
        return false;
    };
    return renderDepTrackerData();
}

void MpkgEngine::commitActions()
{
    if (!core->DepTracker->commitToDb()){
        QStringList removeBL=removeBlacklist();
        PACKAGE_LIST removeList=core->DepTracker->get_remove_list();
        QString details;
        for (int i=0; i<removeList.size(); ++i)
            for (int t=0; t<removeBL.size(); ++t)
                if (QString::fromStdString(removeList[i].get_name())==removeBL[t])
                    details+=tr("Cannot remove package ")+QString::fromStdString(removeList[i].get_name()+"-"+removeList[i].get_fullversion())
                            +tr(", because it is an important system component. ");
        emit sigError(tr("Error"),tr("Found essential packages, cannot continue"),details);
        emit commitFinished();
        return;
    } else {
        core->commit();
    }
    emit commitFinished();
}

void MpkgEngine::updateRepositoryData()
{
    _abortActions=false;
    core->update_repository_data();
    delete_tmp_files();
    if (usedCdromMount) system("umount " + CDROM_MOUNTPOINT + " 2>/dev/null >/dev/null");
    emit updateRepositoryDataFinished();
}

void MpkgEngine::setAbortActions(bool value)
{
    _abortActions=value;
    qDebug()<<"ABORT: "<<value;
}

void MpkgEngine::downloadRepositoryList()
{
    actGetRepositorylist();
    emit sigUpdRep();
}

bool MpkgEngine::renderDepTrackerData()
{
    bool ret=core->DepTracker->renderData()==0;
    if (!ret){
        PACKAGE_LIST failureList=core->DepTracker->get_failure_list();
        QString text=tr("Error: unresolved dependencies ");
        for(int i=0; i<failureList.size(); ++i)
            text+=QString::fromStdString(failureList[i].get_name()+"-"+failureList[i].get_fullversion()+", ");
        text.truncate(text.size()-2);
        text+=". "+QString::number(failureList.size())+tr(" packages has unresolvable dependencies.");
        emit sigError(tr("Dependency error"),text,QString::fromStdString(depErrorTable.print()));
    }
    return ret;
}

QStringList MpkgEngine::toQStringList(const vector<string> &str)
{
    QStringList list;
    vector<string>::const_iterator it;
    for(it = str.begin(); it != str.end(); ++it)
        list.push_back(QString::fromStdString(*it));
    return list;
}

void MpkgEngine::fromQStringList(const QStringList &qlist, vector<string> &str)
{
    str.clear();
    QStringList::const_iterator it;
    for (it = qlist.constBegin(); it != qlist.constEnd(); ++it)
        str.push_back((*it).toStdString());
}

QStringList MpkgEngine::updateBlackList()
{
    vector<string> blacklist = ReadFileStrings("/etc/mpkg-update-blacklist");
    return toQStringList(blacklist);
}

QStringList MpkgEngine::removeBlacklist()
{
    vector<string> blacklist = ReadFileStrings("/etc/mpkg-remove-blacklist");
    return toQStringList(blacklist);
}

void MpkgEngine::setBlacklists(const QStringList &update, const QStringList &remove)
{
    vector<string> updateBL, removeBL;
    fromQStringList(update,updateBL);
    fromQStringList(remove,removeBL);
    WriteFileStrings("/etc/mpkg-update-blacklist", updateBL);
    WriteFileStrings("/etc/mpkg-remove-blacklist", removeBL);
}

bool MpkgEngine::lockDataBase()
{
    return lockDatabase();
}

QString MpkgEngine::createCommitLists(QStringList &install, QStringList &remove, QStringList &upgrade)
{
    PACKAGE_LIST tmpIQueue, tmpRQueue;
    double dl_size=0, ins_size=0, rm_size=0;

    tmpIQueue=core->DepTracker->get_install_list();
    for (unsigned int i=0; i<tmpIQueue.size(); ++i){
        install.push_back(QString::fromStdString(tmpIQueue[i].get_name())+"-"+QString::fromStdString(tmpIQueue[i].get_fullversion()));
        ins_size += strtod(tmpIQueue[i].get_installed_size().c_str(), NULL);
        dl_size += strtod(tmpIQueue[i].get_compressed_size().c_str(), NULL);
    }

    tmpRQueue=core->DepTracker->get_remove_list();
    for (unsigned int i=0; i<tmpRQueue.size(); ++i){
        remove.push_back(QString::fromStdString(tmpRQueue[i].get_name())+"-"+QString::fromStdString(tmpRQueue[i].get_fullversion()));
        rm_size += strtod(tmpRQueue[i].get_installed_size().c_str(), NULL);
    }

    for (unsigned int i=0; i<tmpIQueue.size(); ++i) {
        for (unsigned int t=0; t<tmpRQueue.size(); ++t) {
            if (tmpIQueue[i].get_name()==tmpRQueue[t].get_name()) {
                upgrade.push_back(QString::fromStdString(tmpIQueue[i].get_name()+": "+tmpRQueue[t].get_fullversion()+" ==> "+tmpIQueue[i].get_fullversion()));
            }
        }
    }

    QString diskSpaceDiff;
    if (ins_size==rm_size) diskSpaceDiff=tr("<b>Disk space will be unchanged</b>");
    if (ins_size>rm_size) diskSpaceDiff=tr("<b>Disk space will be occupied: </b>%1").arg(QString::fromStdString(humanizeSize(ins_size-rm_size)));
    if (ins_size<rm_size) diskSpaceDiff=tr("<b>Disk space will be freed: </b>%1").arg(QString::fromStdString(humanizeSize(rm_size-ins_size)));
    QString totalDownloadSize = QString::fromStdString(humanizeSize(dl_size));
    QString actionSummaryLabel=tr("<b>To be installed: </b>%1 packages<br><b>To be removed: </b>%2 packages<br><b>To be upgraded: </b>%3<br><b>Download size: </b>%4<br>%5<br>").arg(install.size()).arg(remove.size()).arg(upgrade.size()).arg(totalDownloadSize).arg(diskSpaceDiff);
    return actionSummaryLabel;
}

void MpkgEngine::getRepositoryList(QStringList &enabledRep, QStringList &disabledRep)
{
    vector<string> enabledRepositories = mpkgconfig::get_repositorylist();
    vector<string> disabledRepositories = mpkgconfig::get_disabled_repositorylist();
    enabledRep=toQStringList(enabledRepositories);
    disabledRep=toQStringList(disabledRepositories);
}

void MpkgEngine::setRepositoryList(const QStringList &enabledRep, const QStringList &disabledRep)
{
    vector<string> enabled, disabled;
    fromQStringList(enabledRep,enabled);
    fromQStringList(disabledRep,disabled);
    mpkgconfig::set_repositorylist(enabled, disabled);
}

QString MpkgEngine::getConfigValue(QString param)
{
    return QString::fromStdString(mConfig.getValue(param.toStdString()));
}

void MpkgEngine::setConfigValue(QString param, QString val)
{
    mConfig.setValue(param.toStdString(),val.toStdString());
}

QStringList MpkgEngine::pkgDistrList()
{
    return packageDistroVersionList;
}
