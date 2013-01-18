#ifndef MPKGTHREAD_H
#define MPKGTHREAD_H

#include <QThread>
#include "mpkgengine.h"

class LoadPackage : public QThread
{
public:
    LoadPackage(MpkgEngine *engine);
    void run();
private:
    MpkgEngine *pEngine;

};

class Commit : public QThread
{
public:
    Commit(MpkgEngine *engine);
    void run();
private:
    MpkgEngine *pEngine;

};

class UpdateRepository : public QThread
{
public:
    UpdateRepository(MpkgEngine *engine);
    void run();
private:
    MpkgEngine *pEngine;

};

#endif // MPKGTHREAD_H
