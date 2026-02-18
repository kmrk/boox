#include "floatingzone.h"
#include <QMouseEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QFileInfo>
#include <QMessageBox>
#include <QPainter>
#include <QDesktopServices>
#include <QHBoxLayout>
#include <QStyle>
#include <QShowEvent>
#include <QDir>
#include <QFontMetrics>
#include <QFile>
#include <QStringList>
#include <QMenu>
#include <QCloseEvent>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include "features/fileops/fileops.h"
#include "features/contextmenu/contextmenu.h"

#ifdef Q_OS_WIN
#include <windows.h>
#endif

FloatingZone::FloatingZone(const QString &name, const QString &folderPath, QWidget *parent)
    : QWidget(parent)
    , zoneName(name)
    , folderPath(folderPath)
    , dragging(false)
    , resizing(false)
    , resizingRight(false)
    , resizingBottom(false)
    , isGridMode(false)
    , folderWatcher(nullptr)
{
    // Set window flags for a frameless window that stays behind other windows
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnBottomHint);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_ShowWithoutActivating, true);  // Don't steal focus when shown

    // Enable drag and drop
    setAcceptDrops(true);

    setupUI();

    // Initialize file system watcher
    folderWatcher = new QFileSystemWatcher(this);
    connect(folderWatcher, &QFileSystemWatcher::directoryChanged,
            this, &FloatingZone::onFolderContentChanged);

    // Set initial size aligned to grid
    QSize initialSize = snapSizeToGrid(QSize(200, 350));
    resize(initialSize);

    // Load files from folder if path is provided
    if (!folderPath.isEmpty()) {
        loadFilesFromFolder();  // This will call updateTitle() and setup watcher
    } else {
        updateTitle();  // Call updateTitle() if no folder
    }

    // Load saved layout (position and size)
    loadLayout();
}

FloatingZone::~FloatingZone()
{
}

void FloatingZone::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(0);  // Remove spacing between title and list

    // Title bar with name only
    QWidget *titleBar = new QWidget(this);
    titleBar->setStyleSheet(
        "QWidget { "
        "  background-color: rgba(0, 0, 0, 51); "
        "  border: 1px solid rgba(255, 255, 255, 50); "
        "  border-bottom: 1px solid rgba(255, 255, 255, 20); "  // Subtle separator
        "}"
    );
    titleBar->setFixedHeight(30);

    QHBoxLayout *titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(10, 2, 10, 2);
    titleLayout->setSpacing(5);

    titleLabel = new ClickableLabel(zoneName, titleBar);
    titleLabel->setStyleSheet(
        "color: white; "
        "font-weight: bold; "
        "font-size: 13px; "
        "background: transparent; "
        "border: none; "
        "padding: 0px; "
        "margin: 0px;"
    );
    connect(titleLabel, &ClickableLabel::doubleClicked, this, &FloatingZone::onTitleDoubleClicked);
    titleLayout->addWidget(titleLabel, 1);

    // Close button
    QPushButton *closeButton = new QPushButton("×", titleBar);
    closeButton->setStyleSheet(
        "QPushButton { "
        "  background-color: transparent; "
        "  color: rgba(255, 255, 255, 150); "
        "  border: none; "
        "  padding: 0px; "
        "  font-size: 20px; "
        "  font-weight: bold; "
        "}"
        "QPushButton:hover { "
        "  background-color: rgba(255, 100, 100, 180); "
        "  color: white; "
        "}"
    );
    closeButton->setFixedSize(24, 22);
    closeButton->setToolTip("关闭区域");
    connect(closeButton, &QPushButton::clicked, this, &FloatingZone::close);
    titleLayout->addWidget(closeButton);

    // View mode toggle button
    viewModeButton = new QPushButton("▦", titleBar);
    viewModeButton->setStyleSheet(
        "QPushButton { "
        "  background-color: rgba(255, 255, 255, 50); "
        "  color: white; "
        "  border: none; "
        "  padding: 2px 6px; "
        "  font-size: 16px; "
        "  font-weight: bold; "
        "}"
        "QPushButton:hover { "
        "  background-color: rgba(255, 255, 255, 80); "
        "}"
    );
    viewModeButton->setFixedSize(24, 22);
    viewModeButton->setToolTip("切换视图模式");
    connect(viewModeButton, &QPushButton::clicked, this, &FloatingZone::toggleViewMode);
    titleLayout->addWidget(viewModeButton);

    mainLayout->addWidget(titleBar);

    // File list in list view with drag support
    fileList = new DraggableListWidget(this);
    fileList->setViewMode(QListWidget::ListMode);
    fileList->setIconSize(QSize(24, 24));
    fileList->setGridSize(QSize());
    fileList->setResizeMode(QListWidget::Adjust);
    fileList->setMovement(QListWidget::Static);
    fileList->setWrapping(false);
    fileList->setSpacing(2);
    fileList->setStyleSheet(
        "QListWidget { "
        "  background-color: rgba(0, 0, 0, 51); "
        "  border: 1px solid rgba(255, 255, 255, 50); "
        "  border-top: none; "
        "  padding: 5px; "
        "}"
        "QListWidget::item { "
        "  background-color: transparent; "
        "  color: white; "
        "  padding: 3px;"
        "}"
        "QListWidget::item:selected { "
        "  background-color: rgba(255, 255, 255, 80); "
        "  color: white; "
        "}"
        "QListWidget::item:hover { "
        "  background-color: rgba(255, 255, 255, 40); "
        "}"
        // Scrollbar styling
        "QScrollBar:vertical { "
        "  background: rgba(0, 0, 0, 30); "
        "  width: 10px; "
        "  border: none; "
        "  margin: 0px; "
        "}"
        "QScrollBar::handle:vertical { "
        "  background: rgba(255, 255, 255, 100); "
        "  min-height: 20px; "
        "  border-radius: 5px; "
        "}"
        "QScrollBar::handle:vertical:hover { "
        "  background: rgba(255, 255, 255, 150); "
        "}"
        "QScrollBar::handle:vertical:pressed { "
        "  background: rgba(255, 255, 255, 200); "
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { "
        "  height: 0px; "
        "}"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { "
        "  background: none; "
        "}"
        // Horizontal scrollbar
        "QScrollBar:horizontal { "
        "  background: rgba(0, 0, 0, 30); "
        "  height: 10px; "
        "  border: none; "
        "  margin: 0px; "
        "}"
        "QScrollBar::handle:horizontal { "
        "  background: rgba(255, 255, 255, 100); "
        "  min-width: 20px; "
        "  border-radius: 5px; "
        "}"
        "QScrollBar::handle:horizontal:hover { "
        "  background: rgba(255, 255, 255, 150); "
        "}"
        "QScrollBar::handle:horizontal:pressed { "
        "  background: rgba(255, 255, 255, 200); "
        "}"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { "
        "  width: 0px; "
        "}"
        "QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal { "
        "  background: none; "
        "}"
    );
    fileList->setContextMenuPolicy(Qt::CustomContextMenu);
    fileList->setWordWrap(true);
    connect(fileList, &QListWidget::customContextMenuRequested, this, &FloatingZone::showContextMenu);

    // Enable drag from list
    fileList->setDragEnabled(true);
    fileList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    fileList->setDragDropMode(QAbstractItemView::DragDrop);
    fileList->setDefaultDropAction(Qt::MoveAction);

    connect(fileList, &QListWidget::itemDoubleClicked, this, &FloatingZone::onItemDoubleClicked);
    connect(fileList, &QListWidget::itemSelectionChanged, this, &FloatingZone::onSelectionChanged);
    mainLayout->addWidget(fileList);

    // No background for main widget (transparent)
    setStyleSheet("FloatingZone { background-color: transparent; }");
}

void FloatingZone::setName(const QString &name)
{
    zoneName = name;
    updateTitle();
}

void FloatingZone::updateTitle()
{
    titleLabel->setText(zoneName);
    setWindowTitle(zoneName);

    // 暂时注销窗口大小自动调整功能
    // Adjust width based on title length and view mode
    // QFontMetrics fm(titleLabel->font());
    // int textWidth = fm.horizontalAdvance(zoneName);
    // int titleWidth = textWidth + 80;  // Title + button space
    //
    // int contentWidth = 200;  // Default minimum
    // if (isGridMode) {
    //     // Grid mode: calculate based on number of items
    //     int itemCount = fileList->count();
    //     if (itemCount > 0) {
    //         // 2 columns minimum, up to 4 columns
    //         int columns = qMin(4, qMax(2, (itemCount + 1) / 2));
    //         contentWidth = columns * 80 + 30;  // grid size + padding
    //     } else {
    //         contentWidth = 180;  // 2 columns default
    //     }
    // } else {
    //     // List mode: calculate based on longest filename
    //     int maxTextWidth = 150;
    //     QFontMetrics itemFm(fileList->font());
    //     for (int i = 0; i < fileList->count(); ++i) {
    //         QListWidgetItem *item = fileList->item(i);
    //         int itemWidth = itemFm.horizontalAdvance(item->text());
    //         maxTextWidth = qMax(maxTextWidth, itemWidth);
    //     }
    //     contentWidth = maxTextWidth + 60;  // text + icon + padding
    // }
    //
    // int newWidth = qMax(titleWidth, contentWidth);
    // newWidth = qMax(200, qMin(500, newWidth));  // Min 200, Max 500
    //
    // // Snap to grid when resizing based on content
    // QSize newSize = snapSizeToGrid(QSize(newWidth, height()));
    // resize(newSize);
}

void FloatingZone::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        QPoint pos = event->pos();

        // Check for resize areas (right edge, bottom edge, or corner)
        bool onRightEdge = pos.x() >= width() - RESIZE_MARGIN;
        bool onBottomEdge = pos.y() >= height() - RESIZE_MARGIN;

        if (onRightEdge || onBottomEdge) {
            resizing = true;
            resizingRight = onRightEdge;
            resizingBottom = onBottomEdge;
            resizeStartPos = event->globalPos();
            resizeStartSize = size();
        } else if (pos.y() < 40) {  // Title bar area
            dragging = true;
            dragPosition = event->globalPos() - frameGeometry().topLeft();
        }

#ifdef Q_OS_WIN
        // Keep window at desktop level
        HWND hwnd = (HWND)winId();
        SetWindowPos(hwnd, HWND_BOTTOM, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
#endif

        event->accept();
    }
}

void FloatingZone::mouseMoveEvent(QMouseEvent *event)
{
    if (dragging) {
        move(event->globalPos() - dragPosition);

#ifdef Q_OS_WIN
        // Keep window at bottom while dragging
        HWND hwnd = (HWND)winId();
        SetWindowPos(hwnd, HWND_BOTTOM, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
#endif
        event->accept();
    } else if (resizing) {
        QPoint delta = event->globalPos() - resizeStartPos;

        int newWidth = resizeStartSize.width();
        int newHeight = resizeStartSize.height();

        if (resizingRight) {
            newWidth = qMax(MIN_WIDTH, resizeStartSize.width() + delta.x());
        }
        if (resizingBottom) {
            newHeight = qMax(MIN_HEIGHT, resizeStartSize.height() + delta.y());
        }

        // Apply grid snapping during resize for real-time feedback
        QSize snappedSize = snapSizeToGrid(QSize(newWidth, newHeight));
        resize(snappedSize);

#ifdef Q_OS_WIN
        // Keep window at bottom while resizing
        HWND hwnd = (HWND)winId();
        SetWindowPos(hwnd, HWND_BOTTOM, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
#endif
        event->accept();
    } else {
        // Change cursor when hovering over resize areas
        QPoint pos = event->pos();
        bool onRightEdge = pos.x() >= width() - RESIZE_MARGIN;
        bool onBottomEdge = pos.y() >= height() - RESIZE_MARGIN;

        if (onRightEdge && onBottomEdge) {
            setCursor(Qt::SizeFDiagCursor);
        } else if (onRightEdge) {
            setCursor(Qt::SizeHorCursor);
        } else if (onBottomEdge) {
            setCursor(Qt::SizeVerCursor);
        } else {
            setCursor(Qt::ArrowCursor);
        }
    }
}

void FloatingZone::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        bool wasMoving = dragging;
        bool wasResizing = resizing;

        // Snap to grid when releasing
        if (dragging) {
            QPoint snappedPos = snapToGrid(pos());
            move(snappedPos);
        }

        if (resizing) {
            QSize snappedSize = snapSizeToGrid(size());
            resize(snappedSize);
        }

        dragging = false;
        resizing = false;
        resizingRight = false;
        resizingBottom = false;
        setCursor(Qt::ArrowCursor);

#ifdef Q_OS_WIN
        // Reset window to desktop level after dragging/resizing
        HWND hwnd = (HWND)winId();
        SetWindowPos(hwnd, HWND_BOTTOM, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
#endif

        // Save layout after moving or resizing
        if (wasMoving || wasResizing) {
            saveLayout();
            emit layoutChanged();
        }

        event->accept();
    }
}

void FloatingZone::dragEnterEvent(QDragEnterEvent *event)
{
    DragDropHandler::handleDragEnter(event);
}

void FloatingZone::dropEvent(QDropEvent *event)
{
    DragDropHandler::handleDrop(event, this);
}

void FloatingZone::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    // Draw subtle resize indicator in the bottom-right corner
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int x = width() - 8;
    int y = height() - 8;
    painter.setPen(QPen(QColor(255, 255, 255, 150), 2));

    // Draw three lines to indicate resize area
    for (int i = 0; i < 3; ++i) {
        painter.drawLine(x - i * 4, y, x, y - i * 4);
    }
}

QRect FloatingZone::getResizeRect() const
{
    return QRect(width() - RESIZE_MARGIN, height() - RESIZE_MARGIN,
                 RESIZE_MARGIN, RESIZE_MARGIN);
}

bool FloatingZone::isInResizeArea(const QPoint &pos) const
{
    return pos.x() >= width() - RESIZE_MARGIN && pos.y() >= height() - RESIZE_MARGIN;
}

void FloatingZone::onItemDoubleClicked(QListWidgetItem *item)
{
    QString filePath = item->data(Qt::UserRole).toString();

    // Open the file or folder with the default system application
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(filePath))) {
        QMessageBox::warning(this, tr("错误"),
                           tr("无法打开文件: %1").arg(filePath));
    }
}

void FloatingZone::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);

#ifdef Q_OS_WIN
    // Set window to desktop level on Windows
    HWND hwnd = (HWND)winId();

    // Set extended window style - no activate, tool window, transparent for mouse
    LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
    SetWindowLong(hwnd, GWL_EXSTYLE, exStyle | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE);

    // Set window position to bottom (behind all normal windows)
    // Use HWND_BOTTOM to keep it below all normal windows
    SetWindowPos(hwnd, HWND_BOTTOM, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
#endif
}

void FloatingZone::closeEvent(QCloseEvent *event)
{
    // Save layout before closing
    saveLayout();

    // Emit signal to notify MainWindow
    emit zoneClosed(this);

    // Accept the close event
    QWidget::closeEvent(event);
}

void FloatingZone::setFolderPath(const QString &path)
{
    folderPath = path;
    loadFilesFromFolder();
}

void FloatingZone::loadFilesFromFolder()
{
    if (folderPath.isEmpty()) {
        return;
    }

    QDir dir(folderPath);
    if (!dir.exists()) {
        return;
    }

    // Setup folder watching
    if (folderWatcher && !folderWatcher->directories().contains(folderPath)) {
        folderWatcher->addPath(folderPath);
    }

    refreshFileList();
    updateTitle();  // Update width after loading files
}

void FloatingZone::refreshFileList()
{
    if (folderPath.isEmpty()) {
        return;
    }

    // Clear existing items
    fileList->clear();

    QDir dir(folderPath);
    if (!dir.exists()) {
        return;
    }

    // Get all files and subdirectories
    QFileInfoList entries = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot,
                                              QDir::Name | QDir::DirsFirst);

    for (const QFileInfo &fileInfo : entries) {
        QListWidgetItem *item = new QListWidgetItem(fileInfo.fileName());
        item->setData(Qt::UserRole, fileInfo.absoluteFilePath());
        item->setToolTip(fileInfo.absoluteFilePath());

        // Set icon based on file type
        if (fileInfo.isDir()) {
            item->setIcon(style()->standardIcon(QStyle::SP_DirIcon));
        } else {
            item->setIcon(style()->standardIcon(QStyle::SP_FileIcon));
        }

        fileList->addItem(item);
    }
}

void FloatingZone::toggleViewMode()
{
    isGridMode = !isGridMode;

    if (isGridMode) {
        // Switch to grid/icon mode
        fileList->setViewMode(QListWidget::IconMode);
        fileList->setIconSize(QSize(48, 48));
        fileList->setGridSize(QSize(80, 90));
        fileList->setWrapping(true);
        fileList->setSpacing(5);
        viewModeButton->setText("≡");
    } else {
        // Switch to list mode
        fileList->setViewMode(QListWidget::ListMode);
        fileList->setIconSize(QSize(24, 24));
        fileList->setGridSize(QSize());
        fileList->setWrapping(false);
        fileList->setSpacing(2);
        viewModeButton->setText("▦");
    }

    // Refresh the file list to apply new view mode
    refreshFileList();

    // Update window width based on new view mode
    updateTitle();
}

void FloatingZone::showContextMenu(const QPoint &pos)
{
    ContextMenuBuilder::show(
        fileList, pos,
        folderPath, zoneName,
        this,
        [this]() { refreshFileList(); },
        [this](const QString &newFolderPath) {
            folderPath = newFolderPath;
            zoneName = QFileInfo(newFolderPath).fileName();
            updateTitle();
            saveLayout();
        }
    );
}


void FloatingZone::onTitleDoubleClicked()
{
    if (folderPath.isEmpty()) {
        return;
    }

    FileOpsHandler::renameFolder(folderPath, zoneName, this,
        [this](const QString &newFolderPath) {
            folderPath = newFolderPath;
            zoneName = QFileInfo(newFolderPath).fileName();
            updateTitle();
            saveLayout();
        }
    );
}

QPoint FloatingZone::snapToGrid(const QPoint &pos) const
{
    int x = qRound(pos.x() / (double)GRID_SIZE) * GRID_SIZE;
    int y = qRound(pos.y() / (double)GRID_SIZE) * GRID_SIZE;
    return QPoint(x, y);
}

QSize FloatingZone::snapSizeToGrid(const QSize &size) const
{
    int width = qMax(MIN_WIDTH, qRound(size.width() / (double)GRID_SIZE) * GRID_SIZE);
    int height = qMax(MIN_HEIGHT, qRound(size.height() / (double)GRID_SIZE) * GRID_SIZE);
    return QSize(width, height);
}

void FloatingZone::saveLayout()
{
    // Only save layout if this zone is linked to a folder
    if (folderPath.isEmpty()) {
        return;
    }

    // Layout file path: d:\boox\.layout.json
    QString layoutFilePath = "d:/boox/.layout.json";

    // Read existing layout data
    QJsonObject layoutData;
    QFile layoutFile(layoutFilePath);
    if (layoutFile.exists()) {
        if (layoutFile.open(QIODevice::ReadOnly)) {
            QByteArray data = layoutFile.readAll();
            layoutFile.close();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            if (doc.isObject()) {
                layoutData = doc.object();
            }
        }
    }

    // Create zone data
    QJsonObject zoneData;
    zoneData["x"] = pos().x();
    zoneData["y"] = pos().y();
    zoneData["width"] = width();
    zoneData["height"] = height();
    zoneData["viewMode"] = isGridMode ? "grid" : "list";

    // Use folder path as key
    layoutData[folderPath] = zoneData;

    // Write back to file
    if (layoutFile.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(layoutData);
        layoutFile.write(doc.toJson(QJsonDocument::Indented));
        layoutFile.close();
    }
}

void FloatingZone::loadLayout()
{
    // Only load layout if this zone is linked to a folder
    if (folderPath.isEmpty()) {
        return;
    }

    // Layout file path: d:\boox\.layout.json
    QString layoutFilePath = "d:/boox/.layout.json";

    QFile layoutFile(layoutFilePath);
    if (!layoutFile.exists()) {
        return;
    }

    if (!layoutFile.open(QIODevice::ReadOnly)) {
        return;
    }

    QByteArray data = layoutFile.readAll();
    layoutFile.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        return;
    }

    QJsonObject layoutData = doc.object();

    // Check if we have saved geometry for this zone
    if (!layoutData.contains(folderPath)) {
        return;
    }

    QJsonValue zoneValue = layoutData[folderPath];
    if (!zoneValue.isObject()) {
        return;
    }

    QJsonObject zoneData = zoneValue.toObject();

    // Restore position and size
    if (zoneData.contains("x") && zoneData.contains("y") &&
        zoneData.contains("width") && zoneData.contains("height")) {
        int x = zoneData["x"].toInt();
        int y = zoneData["y"].toInt();
        int w = zoneData["width"].toInt();
        int h = zoneData["height"].toInt();

        move(x, y);
        resize(w, h);
    }

    // Restore view mode if saved
    if (zoneData.contains("viewMode")) {
        QString viewMode = zoneData["viewMode"].toString();
        bool savedIsGridMode = (viewMode == "grid");

        // Only toggle if different from current mode
        if (savedIsGridMode != isGridMode) {
            toggleViewMode();
        }
    }
}

bool FloatingZone::hasStoredLayout() const
{
    // Only check if this zone is linked to a folder
    if (folderPath.isEmpty()) {
        return false;
    }

    // Layout file path: d:\boox\.layout.json
    QString layoutFilePath = "d:/boox/.layout.json";

    QFile layoutFile(layoutFilePath);
    if (!layoutFile.exists()) {
        return false;
    }

    if (!layoutFile.open(QIODevice::ReadOnly)) {
        return false;
    }

    QByteArray data = layoutFile.readAll();
    layoutFile.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        return false;
    }

    QJsonObject layoutData = doc.object();

    // Check if we have saved geometry for this zone
    return layoutData.contains(folderPath);
}

void FloatingZone::onFolderContentChanged(const QString &path)
{
    Q_UNUSED(path);

    // Re-add the path to watcher in case it was removed (happens on some filesystems)
    if (folderWatcher && !folderWatcher->directories().contains(folderPath)) {
        folderWatcher->addPath(folderPath);
    }

    // Refresh the file list to show new/removed files
    refreshFileList();

    // Update window width if needed based on new content
    updateTitle();
}

void FloatingZone::onSelectionChanged()
{
    QListWidgetItem* item = fileList->currentItem();
    QString selectedPath = item ? item->data(Qt::UserRole).toString() : QString();
    emit selectionChanged(this, selectedPath);
}

void FloatingZone::clearFileSelection()
{
    fileList->clearSelection();
}
