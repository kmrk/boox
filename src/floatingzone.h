#ifndef FLOATINGZONE_H
#define FLOATINGZONE_H

#include <QWidget>
#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>
#include <QPoint>
#include <QPushButton>
#include <QTimer>
#include <QFileSystemWatcher>
#include "features/dragdrop/dragdrop.h"
#include "features/contextmenu/contextmenu.h"
#include "features/fileops/fileops.h"

// Custom QLabel to support double-click
class ClickableLabel : public QLabel
{
    Q_OBJECT
public:
    explicit ClickableLabel(const QString &text, QWidget *parent = nullptr)
        : QLabel(text, parent) {}

protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override
    {
        Q_UNUSED(event);
        emit doubleClicked();
    }

signals:
    void doubleClicked();
};

class FloatingZone : public QWidget
{
    Q_OBJECT

public:
    explicit FloatingZone(const QString &name, const QString &folderPath = QString(), QWidget *parent = nullptr);
    ~FloatingZone();

    QString getName() const { return zoneName; }
    void setName(const QString &name);

    QString getFolderPath() const { return folderPath; }
    void setFolderPath(const QString &path);

    void loadFilesFromFolder();
    void saveLayout();
    void loadLayout();
    bool hasStoredLayout() const;
    
    // For dragdrop feature
    DraggableListWidget* getFileList() const { return fileList; }
    void refreshFileList();
    void clearFileSelection();

signals:
    void zoneClosed(FloatingZone* zone);
    void layoutChanged();
    void selectionChanged(FloatingZone* zone, const QString& selectedPath);

protected:
    // For dragging the window
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

    // For drag and drop files
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

    // For resizing
    void paintEvent(QPaintEvent *event) override;

    // For setting window to desktop level
    void showEvent(QShowEvent *event) override;

    // For saving layout on close
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onItemDoubleClicked(QListWidgetItem *item);
    void toggleViewMode();
    void showContextMenu(const QPoint &pos);
    void onTitleDoubleClicked();
    void onFolderContentChanged(const QString &path);
    void onSelectionChanged();

private:
    void setupUI();
    void updateTitle();
    QRect getResizeRect() const;
    bool isInResizeArea(const QPoint &pos) const;

    QString zoneName;
    QString folderPath;
    ClickableLabel *titleLabel;
    DraggableListWidget *fileList;
    QPushButton *viewModeButton;
    bool isGridMode;
    QFileSystemWatcher *folderWatcher;

    // For window dragging
    bool dragging;
    QPoint dragPosition;

    // For window resizing
    bool resizing;
    bool resizingRight;
    bool resizingBottom;
    QPoint resizeStartPos;
    QSize resizeStartSize;

    static constexpr int RESIZE_MARGIN = 20;  // Increased for easier resizing
    static constexpr int MIN_WIDTH = 200;
    static constexpr int MIN_HEIGHT = 150;
    static constexpr int GRID_SIZE = 50;  // Grid snap size in pixels

    QPoint snapToGrid(const QPoint &pos) const;
    QSize snapSizeToGrid(const QSize &size) const;
};

#endif // FLOATINGZONE_H
