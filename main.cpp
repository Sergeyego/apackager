#include <QApplication>
#include "mainwindow.h"
#include <mpkg/libmpkg.h>

int main(int argc, char *argv[])
{
    if (getuid()) {
        // trying to obtain root UID
        setuid(0);
        if (getuid()) {
            string args;
            for (int i=1; i<argc; ++i) {
                args += string(argv[i]) + " ";
            }
            return system("xdg-su -c \"" + string(argv[0]) + " " + args + "\"");
        }
    }

    setlocale(LC_ALL, "");
    bindtextdomain( "mpkg", "/usr/share/locale");
    textdomain("mpkg");

    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
    QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));

    QStringList fname;
    for (int i=1; i<argc; i++) {
        fname.push_back(argv[i]);
    }

    QApplication a(argc, argv);

    QLocale lc;
    QTranslator translator;
    translator.load("mpkg_manager_" + lc.name(), "/usr/share/mpkg/apackager");
    a.installTranslator(&translator);

    MainWindow w;
    w.show();
    if (fname.isEmpty()){
        w.updateSlot();
    } else {
        w.installFiles(fname);
    }
    return a.exec();
}
