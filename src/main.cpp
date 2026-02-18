#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("Boox");
    a.setOrganizationName("Boox");

    MainWindow w;
    // MainWindow is hidden - only tray icon is visible

    return a.exec();
}
