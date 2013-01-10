#ifndef MPKGHANDLER_H
#define MPKGHANDLER_H

#include <QObject>
#include <mpkg/errorhandler.h>
#include <mpkg/bus.h>
#include <QDebug>
#include <QStringList>
#include <QWaitCondition>
#include <QMutex>

typedef struct {
    QString name;
    QString currentAction;
    int progress;
    int totalProgress;
} progressMessage;

typedef struct {
    QString error;
    QString informativeText;
    QStringList textButtons;
} errorMessage;

class MpkgHandler : public QObject
{
    Q_OBJECT
public:
    ~MpkgHandler();
    static MpkgHandler* getInstance();
    static void updateProgressData(ItemState a);
    static MpkgErrorReturn qtErrorHandler(ErrorDescription err, const string& details);
signals:
    void sigErr(errorMessage message);
    void sigProgress(progressMessage message);
protected:
    MpkgHandler(QObject *parent = 0);
private:
    static MpkgHandler* handler_instance;
    void emitErrSignal(errorMessage message);
    void emitMessSignal(progressMessage message);
    int clickButton;
    QWaitCondition buttonPressed;
    QMutex mutex;

public slots:
    void clickButtonSlot(int button);
};

#endif // MPKGHANDLER_H
