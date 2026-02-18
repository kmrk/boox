#include "mainwindow.h"
#include <QApplication>
#include <QMessageBox>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QGuiApplication>
#include <QScreen>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , trayIcon(nullptr)
    , trayMenu(nullptr)
    , fileWatcher(nullptr)
    , zoneCounter(1)
    , booxRootPath("d:/boox")
{
    // Hide the main window - we only use tray icon
    hide();

    createActions();
    setupTrayIcon();

    // Initialize and scan the boox directory
    initializeBooxDirectory();
    scanBooxDirectory();
}

MainWindow::~MainWindow()
{
    // Clean up zones
    qDeleteAll(zones);
    zones.clear();

    // Clean up file watcher
    if (fileWatcher) {
        delete fileWatcher;
    }
}

void MainWindow::createActions()
{
    newZoneAction = new QAction(tr("新建悬浮区域(&N)"), this);
    connect(newZoneAction, &QAction::triggered, this, &MainWindow::createNewZone);

    showAllAction = new QAction(tr("显示所有区域(&S)"), this);
    connect(showAllAction, &QAction::triggered, this, &MainWindow::showAllZones);

    hideAllAction = new QAction(tr("隐藏所有区域(&H)"), this);
    connect(hideAllAction, &QAction::triggered, this, &MainWindow::hideAllZones);

    quitAction = new QAction(tr("退出(&Q)"), this);
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);
}

void MainWindow::setupTrayIcon()
{
    trayMenu = new QMenu(this);
    trayMenu->addAction(newZoneAction);
    trayMenu->addSeparator();
    trayMenu->addAction(showAllAction);
    trayMenu->addAction(hideAllAction);
    trayMenu->addSeparator();
    trayMenu->addAction(quitAction);

    trayIcon = new QSystemTrayIcon(this);
    // Use a default icon - you can replace this with your own icon
    trayIcon->setIcon(QApplication::style()->standardIcon(QStyle::SP_ComputerIcon));
    trayIcon->setContextMenu(trayMenu);
    trayIcon->setToolTip(tr("Boox - 桌面悬浮区域"));

    connect(trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::trayIconActivated);

    trayIcon->show();
}

void MainWindow::createNewZone()
{
    // Generate unique folder name
    QString zoneName = tr("区域 %1").arg(zoneCounter++);
    QString folderPath = QDir(booxRootPath).absoluteFilePath(zoneName);

    // Check if folder already exists, find a unique name
    while (QDir(folderPath).exists()) {
        zoneName = tr("区域 %1").arg(zoneCounter++);
        folderPath = QDir(booxRootPath).absoluteFilePath(zoneName);
    }

    // Temporarily block file system watcher signals to prevent duplicate zone creation
    fileWatcher->blockSignals(true);

    // Create the physical folder
    QDir dir;
    if (!dir.mkpath(folderPath)) {
        fileWatcher->blockSignals(false);  // Restore signals
        QMessageBox::warning(nullptr, tr("错误"),
                           tr("无法创建文件夹: %1").arg(folderPath));
        return;
    }

    // Create zone with folder path
    FloatingZone *zone = new FloatingZone(zoneName, folderPath);

    connect(zone, &FloatingZone::zoneClosed, this, &MainWindow::onZoneClosed);
    connect(zone, &FloatingZone::selectionChanged, this, &MainWindow::onZoneSelectionChanged);

    zones.append(zone);

    // Position new zone if no stored layout
    if (!zone->hasStoredLayout()) {
        int index = zones.size() - 1;

        QRect screenGeometry = QGuiApplication::primaryScreen()->availableGeometry();
        int screenWidth = screenGeometry.width();
        int screenHeight = screenGeometry.height();

        const int GRID_SIZE = 50;
        const int ZONE_HEIGHT = 350;
        const int MARGIN = 50;

        int zonesPerColumn = qMax(1, (screenHeight - MARGIN * 2) / (ZONE_HEIGHT + GRID_SIZE));
        int col = index / zonesPerColumn;
        int row = index % zonesPerColumn;

        int xPos = screenWidth - (col + 1) * (zone->width() + MARGIN);
        int yPos = MARGIN + row * (ZONE_HEIGHT + GRID_SIZE);

        xPos = qRound(xPos / (double)GRID_SIZE) * GRID_SIZE;
        yPos = qRound(yPos / (double)GRID_SIZE) * GRID_SIZE;

        zone->move(xPos, yPos);
    }

    zone->show();

    // Restore file system watcher signals
    fileWatcher->blockSignals(false);

    trayIcon->showMessage(tr("新建区域"),
                          tr("已创建新的悬浮区域: %1").arg(zoneName),
                          QSystemTrayIcon::Information, 2000);
}

void MainWindow::showAllZones()
{
    for (FloatingZone *zone : zones) {
        zone->show();
    }
}

void MainWindow::hideAllZones()
{
    for (FloatingZone *zone : zones) {
        zone->hide();
    }
}

void MainWindow::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::DoubleClick) {
        createNewZone();
    }
}

void MainWindow::onZoneClosed(FloatingZone* zone)
{
    zones.removeOne(zone);
    zone->deleteLater();
}

void MainWindow::initializeBooxDirectory()
{
    QDir dir(booxRootPath);
    if (!dir.exists()) {
        if (dir.mkpath(booxRootPath)) {
            trayIcon->showMessage(tr("初始化完成"),
                                  tr("已创建 Boox 目录: %1").arg(booxRootPath),
                                  QSystemTrayIcon::Information, 2000);
        } else {
            QMessageBox::warning(nullptr, tr("错误"),
                               tr("无法创建 Boox 目录: %1").arg(booxRootPath));
            return;
        }
    }

    // Setup file system watcher
    fileWatcher = new QFileSystemWatcher(this);
    fileWatcher->addPath(booxRootPath);
    connect(fileWatcher, &QFileSystemWatcher::directoryChanged,
            this, &MainWindow::onBooxDirectoryChanged);
}

void MainWindow::scanBooxDirectory()
{
    QDir dir(booxRootPath);
    if (!dir.exists()) {
        return;
    }

    // Get all subdirectories in the boox root
    QFileInfoList folders = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);

    if (folders.isEmpty()) {
        trayIcon->showMessage(tr("提示"),
                             tr("Boox 目录为空,请在 %1 下创建文件夹").arg(booxRootPath),
                             QSystemTrayIcon::Information, 3000);
        return;
    }

    // Create a zone for each folder
    for (const QFileInfo &folderInfo : folders) {
        createZoneForFolder(folderInfo.absoluteFilePath());
    }
}

void MainWindow::createZoneForFolder(const QString &folderPath)
{
    QFileInfo folderInfo(folderPath);
    if (!folderInfo.exists() || !folderInfo.isDir()) {
        return;
    }

    // Check if a zone already exists for this folder
    for (FloatingZone *zone : zones) {
        if (zone->getFolderPath() == folderPath) {
            return; // Zone already exists
        }
    }

    QString folderName = folderInfo.fileName();
    FloatingZone *zone = new FloatingZone(folderName, folderPath);

    connect(zone, &FloatingZone::zoneClosed, this, &MainWindow::onZoneClosed);
    connect(zone, &FloatingZone::selectionChanged, this, &MainWindow::onZoneSelectionChanged);

    zones.append(zone);

    // Only set default position if there's no stored layout
    if (!zone->hasStoredLayout()) {
        // Position zones in a grid layout on the right side of the screen
        int index = zones.size() - 1;

        // Get screen geometry
        QRect screenGeometry = QGuiApplication::primaryScreen()->availableGeometry();
        int screenWidth = screenGeometry.width();
        int screenHeight = screenGeometry.height();

        // Grid settings
        const int GRID_SIZE = 50;  // Match the GRID_SIZE in FloatingZone
        const int ZONE_HEIGHT = 350;  // Default zone height
        const int MARGIN = 50;  // Margin from screen edge (aligned to grid)

        // Calculate grid-aligned position from right side
        int zonesPerColumn = qMax(1, (screenHeight - MARGIN * 2) / (ZONE_HEIGHT + GRID_SIZE));
        int col = index / zonesPerColumn;
        int row = index % zonesPerColumn;

        // Calculate position and align to grid
        int xPos = screenWidth - (col + 1) * (zone->width() + MARGIN);
        int yPos = MARGIN + row * (ZONE_HEIGHT + GRID_SIZE);

        // Snap to grid
        xPos = qRound(xPos / (double)GRID_SIZE) * GRID_SIZE;
        yPos = qRound(yPos / (double)GRID_SIZE) * GRID_SIZE;

        zone->move(xPos, yPos);
    }

    zone->show();
}

void MainWindow::onBooxDirectoryChanged(const QString &path)
{
    Q_UNUSED(path);

    // Rescan the directory to pick up new folders
    // This is a simple implementation - you might want to optimize it
    // to only handle added/removed folders instead of rescanning everything

    QDir dir(booxRootPath);
    QFileInfoList folders = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);

    // Check for new folders and create zones
    for (const QFileInfo &folderInfo : folders) {
        createZoneForFolder(folderInfo.absoluteFilePath());
    }

    // Check for deleted folders and remove corresponding zones
    QList<FloatingZone*> zonesToRemove;
    for (FloatingZone *zone : zones) {
        QString zonePath = zone->getFolderPath();
        if (!zonePath.isEmpty() && !QDir(zonePath).exists()) {
            zonesToRemove.append(zone);
        }
    }

    for (FloatingZone *zone : zonesToRemove) {
        zones.removeOne(zone);
        zone->close();
        zone->deleteLater();
    }
}

void MainWindow::onZoneSelectionChanged(FloatingZone* changedZone, const QString& selectedPath)
{
    Q_UNUSED(selectedPath);

    for (FloatingZone* zone : zones) {
        if (zone != changedZone) {
            zone->clearFileSelection();
        }
    }
}
