#ifndef MPKGENGINE_H
#define MPKGENGINE_H

#include <QObject>
#include <mpkg/libmpkg.h>
#include <mpkg/menu.h>
#include <QDebug>
#include <QStringList>
#include <QVector>
#include <QThread>
#include "mpkghandler.h"

enum {
    ICONSTATE_UNKNOWN = 0,
    ICONSTATE_INSTALLED,
    ICONSTATE_INSTALL,
    ICONSTATE_AVAILABLE,
    ICONSTATE_REMOVE,
    ICONSTATE_INSTALLED_DEPRECATED,
    ICONSTATE_AVAILABLE_DEPRECATED,
    ICONSTATE_UPDATE
};

typedef struct{
    QString name;
    QString version;
    QString short_discription;    
    QStringList tags;
    int state;
    QString packageDistroVersion;
} mpkgPackage;


class MpkgEngine : public QObject
{
    Q_OBJECT
public:
    MpkgEngine(QObject *parent = 0);
    ~MpkgEngine();
    QVector<mpkgPackage> cacheBase;
    void updatePkgList();
    QString getPkgInfo(const int &index) const;
    QString getPkgDependencies(const int &index) const;
    bool pkgIsInstalled(const int &index);
    QStringList availableTags();
    QStringList updateBlackList();
    QStringList removeBlacklist();
    void setBlacklists(const QStringList &update, const QStringList &remove);
    bool createQueue(const QVector <int> &pkgInst, const QVector <int> &pkgRem);
    bool createQueue(const QStringList &fname);
    void commitActions();
    void pushErrorButton(int button);
    bool lockDataBase();
    QString createCommitLists(QStringList &install, QStringList &remove, QStringList &upgrade);
    void getRepositoryList(QStringList &enabledRep, QStringList &disabledRep);
    void setRepositoryList(const QStringList &enabledRep, const QStringList &disabledRep);
    QString getConfigValue(QString param);
    void setConfigValue(QString param, QString val);
    QStringList pkgDistrList();
    
signals:
    void updateFinished();
    void commitFinished();
    void updateRepositoryDataFinished();
    void sigError(QString title, QString errText, QString details);
    void sigUpdRep();
    
public slots:
    void resetQueue();
    void cleanCache();
    void updateRepositoryData();
    void setAbortActions(bool value);
    void downloadRepositoryList();

private:
    mpkg *core;
    PACKAGE_LIST packageList;
    QStringList tagsList;
    bool renderDepTrackerData();
    QStringList toQStringList(const vector<string> &str);
    void fromQStringList(const QStringList &qlist, vector<string> &str);
    QStringList packageDistroVersionList;
};

#endif // MPKGENGINE_H
