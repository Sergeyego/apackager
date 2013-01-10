#include "mpkgthread.h"

LoadPackage::LoadPackage(MpkgEngine *engine)
{
    pEngine=engine;
}

void LoadPackage::run()
{
    pEngine->updatePkgList();
}


Commit::Commit(MpkgEngine *engine)
{
    pEngine=engine;
}

void Commit::run()
{
    pEngine->commitActions();
}


UpdateRepository::UpdateRepository(MpkgEngine *engine)
{
    pEngine=engine;
}

void UpdateRepository::run()
{
    pEngine->updateRepositoryData();
}
