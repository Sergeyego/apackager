#include "mpkghandler.h"

MpkgHandler* MpkgHandler::handler_instance = 0;

MpkgHandler::MpkgHandler(QObject *parent) :
    QObject(parent)
{
    qRegisterMetaType<progressMessage>("progressMessage");
    qRegisterMetaType<errorMessage>("errorMessage");
    clickButton=-1;
}

void MpkgHandler::emitErrSignal(errorMessage message)
{
    emit sigErr(message);
}

void MpkgHandler::emitMessSignal(progressMessage message)
{
    emit sigProgress(message);
}

void MpkgHandler::clickButtonSlot(int button)
{
    clickButton=button;
    buttonPressed.wakeAll();
}

MpkgHandler *MpkgHandler::getInstance()
{
    if (handler_instance == 0)
        handler_instance = new MpkgHandler;
    return handler_instance;
}

void MpkgHandler::updateProgressData(ItemState a)
{
    qDebug()<<"PROGRESS: "<<QString::fromStdString(a.currentAction)<<" "<<QString::fromStdString(a.name)<<" "<<a.progress<<" "<<a.totalProgress;
    progressMessage message;
    message.name=QString::fromStdString(a.name);
    message.currentAction=QString::fromStdString(a.currentAction);
    message.progress=a.progress;
    message.totalProgress=a.totalProgress;
    getInstance()->emitMessSignal(message);
}

MpkgErrorReturn MpkgHandler::qtErrorHandler(ErrorDescription err, const string &details)
{
    getInstance()->clickButton=-1;
    errorMessage message;
    message.error= QString::fromStdString(err.text);
    message.informativeText= QString::fromStdString(details);
    for (int i=0; i<err.action.size(); ++i)
        message.textButtons.push_back(QString::fromStdString(err.action[i].text));
    qDebug()<<"ERROR: "<<message.error;
    getInstance()->emitErrSignal(message);
    getInstance()->buttonPressed.wait(&(getInstance()->mutex));
    int ret=getInstance()->clickButton;
    qDebug()<<"CLICKED BUTTON: "<<ret;
    return message.textButtons.size()? err.action[ret].ret : MPKG_RETURN_ABORT;
}

MpkgHandler::~MpkgHandler()
{
}
