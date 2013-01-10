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
    vector<DEPENDENCY> deps;
    PACKAGE p;
    QString linkList;
    QString tagList;
    QString info;
    int lastIndex, installedIndex;
    int state;

    for (unsigned int i=0; i< n; ++i){
        p=packageList[i];
        pkg.name=QString::fromStdString(p.get_name());
        pkg.version=QString::fromStdString(p.get_fullversion());
        pkg.short_discription=QString::fromStdString(p.get_short_description());

        pkg.tags=":";
        tags = p.get_tags();
        for (unsigned int n=0; n<tags.size(); ++n) {
            pkg.tags+=(QString::fromStdString(tags[n]))+":";
        }

        lastIndex=packageList.getMaxVersionNumber(p.get_name());
        installedIndex=packageList.getInstalledVersionNumber(p.get_name());
        if (p.installed()) {
            state = ((i==lastIndex)&&(i==installedIndex))? ICONSTATE_INSTALLED : ICONSTATE_INSTALLED_DEPRECATED;
        } else if (p.available()) {
            if (installedIndex!=-1) {
                state = (i==lastIndex)? ICONSTATE_UPDATE : ICONSTATE_AVAILABLE_DEPRECATED;
            } else {
                state = (i==lastIndex)? ICONSTATE_AVAILABLE : ICONSTATE_AVAILABLE_DEPRECATED;
            }
        } else {
            state = ICONSTATE_UNKNOWN;
        }
        pkg.state=state;

        linkList.clear();
        for (unsigned int n=0; n<p.get_locations().size(); ++n) {
            linkList.push_back("<a href=\"" + QString::fromStdString(p.get_locations().at(n).get_full_url() + p.get_filename()) + "\">" + QString::fromStdString(p.get_filename()) + "</a><br>");
        }
        tagList.clear();
        for (unsigned int n=0; n<tags.size(); ++n) {
            tagList.push_back(QString::fromStdString(tags.at(n)));
                if (n<tags.size()-1) tagList += " ";
        }
        tagList += "<br>";
        if (!tagList.isEmpty()) tagList = "<hr><big><b>" + tr("Tags") + ":</b></big> " + tagList;
        info = ("<b><big>" + pkg.name + " " + pkg.version + "</b></big><br><hr>" + QString::fromStdString(p.get_description()) + \
                "<hr><b>" + tr("Package size: ") + "</b>" + QString::fromStdString(humanizeSize(p.get_compressed_size())) + \
                "<br><b>" + tr("Installed size: ") + "</b>" + QString::fromStdString(humanizeSize(p.get_installed_size())) + \
                "<br><b>" + tr("Distrib: ") + "</b>" + QString::fromStdString(p.package_distro_version) + \
                "<br><b>" + tr("Maintainer: ") + "</b>" + QString::fromStdString(p.get_packager()) + \
                "<br><b>" + tr("File name: ") + "</b>" + QString::fromStdString(p.get_filename()) + \
                "<br><b>MD5: </b>" + QString::fromStdString(p.get_md5()) +
                tagList + "<hr><big><b>" + tr("Download links:") + "</b></big><br>" + linkList);
        pkg.info=info;

        deps=p.get_dependencies();
        pkg.dependency.clear();
        for (unsigned int n=0; n<deps.size(); ++n) {
            pkg.dependency.push_back(QString::fromStdString(deps.at(n).getDepInfo())+"<br>");
        }
        cacheBase[i]=pkg;
    }
    vector<string> available_tags;
    core->get_available_tags(&available_tags);
    tagsList.clear();
    sort(available_tags.begin(), available_tags.end());
    for (unsigned int i=0; i<available_tags.size(); ++i) {
        tagsList.push_back(QString::fromStdString(available_tags[i]));
    }
    qDebug()<<"update finished";
    emit updateFinished();
}


bool MpkgEngine::pkgIsInstalled(int index)
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

bool MpkgEngine::createQueue(QVector <int> pkgInst, QVector <int> pkgRem)
{
    PACKAGE_LIST tmpIQueue, tmpRQueue;

    for (unsigned int i=0; i<pkgInst.size(); ++i){
        tmpIQueue.add(packageList[pkgInst[i]]);
    }

    for (unsigned int i=0; i<pkgRem.size(); ++i){
        tmpRQueue.add(packageList[pkgRem[i]]);
    }

    _abortActions=false;
    resetQueue();
    core->install(&tmpIQueue);
    core->uninstall(&tmpRQueue);
    return renderDepTrackerData();
}

bool MpkgEngine::createQueue(QStringList &fname)
{
    vector<string> pkg, errors;
    QString errText;
    _abortActions=false;
    resetQueue();
    for (int i=0; i<fname.size(); ++i) pkg.push_back(fname.at(i).toStdString());
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
        emit commitFinished();
    }
    return ret;
}

QStringList MpkgEngine::updateBlackList()
{
    QStringList list;
    vector<string> blacklist = ReadFileStrings("/etc/mpkg-update-blacklist");
    for (int i=0; i<blacklist.size(); ++i)
        list.push_back(QString::fromStdString(blacklist.at(i)));
    return list;
}

QStringList MpkgEngine::removeBlacklist()
{
    QStringList list;
    vector<string> blacklist = ReadFileStrings("/etc/mpkg-remove-blacklist");
    for (int i=0; i<blacklist.size(); ++i)
        list.push_back(QString::fromStdString(blacklist.at(i)));
    return list;
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
