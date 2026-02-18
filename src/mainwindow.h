#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QList>
#include <QFileSystemWatcher>
#include "floatingzone.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void createNewZone();
    void showAllZones();
    void hideAllZones();
    void trayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void onZoneClosed(FloatingZone* zone);
    void onBooxDirectoryChanged(const QString &path);
    void onZoneSelectionChanged(FloatingZone* changedZone, const QString& selectedPath);

private:
    void setupTrayIcon();
    void createActions();
    void initializeBooxDirectory();
    void scanBooxDirectory();
    void createZoneForFolder(const QString &folderPath);

    QSystemTrayIcon *trayIcon;
    QMenu *trayMenu;
    QList<FloatingZone*> zones;
    QFileSystemWatcher *fileWatcher;

    QAction *newZoneAction;
    QAction *showAllAction;
    QAction *hideAllAction;
    QAction *quitAction;

    int zoneCounter;
    QString booxRootPath;
};

#endif // MAINWINDOW_H
