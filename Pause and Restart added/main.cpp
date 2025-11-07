#include "mainwindow.h"
#include <QApplication>
#include <QPixmapCache>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    QPixmapCache::setCacheLimit(128 * 4096);
    MainWindow w;
    w.show();
    return a.exec();
}//
