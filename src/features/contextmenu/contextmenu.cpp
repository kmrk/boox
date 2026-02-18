#include "contextmenu.h"
#include "../fileops/fileops.h"
#include <QListWidget>
#include <QListWidgetItem>
#include <QMenu>
#include <QAction>
#include <QObject>

static const char *MENU_STYLE =
    "QMenu { "
    "  background-color: rgba(40, 40, 40, 240); "
    "  color: white; "
    "  border: 1px solid rgba(255, 255, 255, 50); "
    "  border-radius: 5px; "
    "  padding: 5px; "
    "}"
    "QMenu::item { "
    "  padding: 5px 20px; "
    "  border-radius: 3px; "
    "}"
    "QMenu::item:selected { "
    "  background-color: rgba(255, 255, 255, 80); "
    "}";

static QMenu *makeMenu(QWidget *parent)
{
    QMenu *menu = new QMenu(parent);
    menu->setStyleSheet(MENU_STYLE);
    return menu;
}

void ContextMenuBuilder::show(QListWidget *fileList, const QPoint &pos,
                              const QString &zoneFolderPath, const QString &zoneName,
                              QWidget *parent,
                              std::function<void()> onRefresh,
                              std::function<void(const QString &newFolderPath)> onRenamed)
{
    QListWidgetItem *item = fileList->itemAt(pos);

    if (item) {
        const QString filePath = item->data(Qt::UserRole).toString();
        showFileMenu(filePath, zoneFolderPath, zoneName,
                     fileList, pos, parent, onRefresh, onRenamed);
    } else {
        showBlankMenu(zoneFolderPath, zoneName,
                      fileList, pos, parent, onRefresh, onRenamed);
    }
}

void ContextMenuBuilder::showFileMenu(const QString &filePath,
                                      const QString &zoneFolderPath, const QString &zoneName,
                                      QListWidget *fileList, const QPoint &pos,
                                      QWidget *parent,
                                      std::function<void()> onRefresh,
                                      std::function<void(const QString &newFolderPath)> onRenamed)
{
    QMenu *menu = makeMenu(parent);

    QAction *openAction     = menu->addAction(QObject::tr("打开"));
    QAction *copyPathAction = menu->addAction(QObject::tr("复制路径"));
    menu->addSeparator();
    QAction *renameZoneAction = menu->addAction(QObject::tr("重命名区域"));
    menu->addSeparator();
    QAction *deleteAction   = menu->addAction(QObject::tr("删除"));

    QObject::connect(openAction, &QAction::triggered, [filePath, parent]() {
        FileOpsHandler::openFile(filePath, parent);
    });
    QObject::connect(copyPathAction, &QAction::triggered, [filePath]() {
        FileOpsHandler::copyFilePath(filePath);
    });
    QObject::connect(renameZoneAction, &QAction::triggered,
                     [zoneFolderPath, zoneName, parent, onRenamed]() {
        FileOpsHandler::renameFolder(zoneFolderPath, zoneName, parent, onRenamed);
    });
    QObject::connect(deleteAction, &QAction::triggered, [filePath, parent, onRefresh]() {
        FileOpsHandler::deleteFile(filePath, parent, onRefresh);
    });

    menu->exec(fileList->mapToGlobal(pos));
    menu->deleteLater();
}

void ContextMenuBuilder::showBlankMenu(const QString &zoneFolderPath, const QString &zoneName,
                                       QListWidget *fileList, const QPoint &pos,
                                       QWidget *parent,
                                       std::function<void()> onRefresh,
                                       std::function<void(const QString &newFolderPath)> onRenamed)
{
    QMenu *menu = makeMenu(parent);

    QAction *newFileAction   = menu->addAction(QObject::tr("新建文件"));
    QAction *newFolderAction = menu->addAction(QObject::tr("新建文件夹"));
    menu->addSeparator();
    QAction *renameZoneAction = menu->addAction(QObject::tr("重命名区域"));

    QObject::connect(newFileAction, &QAction::triggered, [zoneFolderPath, parent, onRefresh]() {
        FileOpsHandler::createFile(zoneFolderPath, parent, onRefresh);
    });
    QObject::connect(newFolderAction, &QAction::triggered, [zoneFolderPath, parent, onRefresh]() {
        FileOpsHandler::createFolder(zoneFolderPath, parent, onRefresh);
    });
    QObject::connect(renameZoneAction, &QAction::triggered,
                     [zoneFolderPath, zoneName, parent, onRenamed]() {
        FileOpsHandler::renameFolder(zoneFolderPath, zoneName, parent, onRenamed);
    });

    menu->exec(fileList->mapToGlobal(pos));
    menu->deleteLater();
}
