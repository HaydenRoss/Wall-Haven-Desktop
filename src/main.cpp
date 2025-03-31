#include "mainwindow.h"

#include <QApplication>
/*
#include <QPointer>

static QPointer<QNetworkAccessManager> globalManager;

QNetworkAccessManager *nMgr(){
    return globalManager;
}
*/

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    //QNetworkAccessManager mgr;
    //globalManager = &mgr;

    MainWindow w;
    w.show();
    return a.exec();
}
