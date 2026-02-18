#ifndef CONTEXTMENU_H
#define CONTEXTMENU_H

#include <QPoint>
#include <functional>

class QListWidget;
class QWidget;

// Builds and shows the right-click context menu for the file list.
// - Click on a file/folder: file operations menu (open, copy path, delete)
// - Click on empty space:   create menu (new file, new folder) + zone rename
// Delegates actual file operations to FileOpsHandler.
class ContextMenuBuilder
{
public:
    // Show context menu at pos (in fileList local coordinates).
    // onRefresh: called after any operation that modifies the file list.
    // onRenamed(newFolderPath): called after zone folder rename.
    static void show(QListWidget *fileList, const QPoint &pos,
                     const QString &zoneFolderPath, const QString &zoneName,
                     QWidget *parent,
                     std::function<void()> onRefresh,
                     std::function<void(const QString &newFolderPath)> onRenamed);

private:
    static void showFileMenu(const QString &filePath,
                             const QString &zoneFolderPath, const QString &zoneName,
                             QListWidget *fileList, const QPoint &pos,
                             QWidget *parent,
                             std::function<void()> onRefresh,
                             std::function<void(const QString &newFolderPath)> onRenamed);

    static void showBlankMenu(const QString &zoneFolderPath, const QString &zoneName,
                              QListWidget *fileList, const QPoint &pos,
                              QWidget *parent,
                              std::function<void()> onRefresh,
                              std::function<void(const QString &newFolderPath)> onRenamed);

};

#endif // CONTEXTMENU_H
