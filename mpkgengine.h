#ifndef MPKGENGINE_H
#define MPKGENGINE_H

#include <QObject>
#include <mpkg/libmpkg.h>
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
    QString tags;
    int state;
    QString info;
    QString dependency;
} mpkgPackage;


class MpkgEngine : public QObject
{
    Q_OBJECT
public:
    MpkgEngine(QObject *parent = 0);
    ~MpkgEngine();
    QVector<mpkgPackage> cacheBase;
    void updatePkgList();
    bool pkgIsInstalled(int index);
    QStringList availableTags();
    QStringList updateBlackList();
    QStringList removeBlacklist();
    bool createQueue(QVector <int> pkgInst, QVector <int> pkgRem);
    bool createQueue(QStringList &fname);
    void commitActions();
    void pushErrorButton(int button);
    bool lockDataBase();
    QString createCommitLists(QStringList &install, QStringList &remove, QStringList &upgrade);
    
signals:
    void updateFinished();
    void commitFinished();
    void updateRepositoryDataFinished();
    void sigError(QString title, QString errText, QString details);
    
public slots:
    void resetQueue();
    void cleanCache();
    void updateRepositoryData();
    void setAbortActions(bool value);

private:
    mpkg *core;
    PACKAGE_LIST packageList;
    QStringList tagsList;
    bool renderDepTrackerData();
};

#endif // MPKGENGINE_H
