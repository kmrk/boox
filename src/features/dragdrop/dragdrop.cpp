#include "dragdrop.h"
#include <QUrl>
#include <QFileInfo>
#include <QMessageBox>
#include <QDir>
#include <QFile>
#include <QStringList>
#include <QObject>
#include "../../floatingzone.h"

// Implementation of DraggableListWidget
DraggableListWidget::DraggableListWidget(QWidget *parent) : QListWidget(parent)
{
    setAcceptDrops(true);
    setDragDropMode(QAbstractItemView::DragDrop);
    setDefaultDropAction(Qt::MoveAction);
}

QMimeData* DraggableListWidget::mimeData(const QList<QListWidgetItem*> items) const
{
    QMimeData *mimeData = new QMimeData();
    QList<QUrl> urls;

    for (QListWidgetItem *item : items) {
        QString filePath = item->data(Qt::UserRole).toString();
        if (!filePath.isEmpty()) {
            urls.append(QUrl::fromLocalFile(filePath));
        }
    }

    if (!urls.isEmpty()) {
        mimeData->setUrls(urls);
    }

    return mimeData;
}

void DraggableListWidget::dragEnterEvent(QDragEnterEvent *event)
{
    // Accept drops if we have URLs (files from external or internal sources)
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    } else {
        QListWidget::dragEnterEvent(event);
    }
}

void DraggableListWidget::dragMoveEvent(QDragMoveEvent *event)
{
    if (!event->mimeData()->hasUrls()) {
        event->ignore();
        return;
    }

    // Get the item under the cursor (use pos() for Qt5/Qt6 compatibility)
    QListWidgetItem *item = itemAt(event->pos());

    if (item) {
        QString filePath = item->data(Qt::UserRole).toString();
        QFileInfo fileInfo(filePath);

        // Only accept drops on folders
        if (fileInfo.isDir()) {
            event->acceptProposedAction();
            // Highlight the folder item
            setCurrentItem(item);
        } else {
            event->ignore();
        }
    } else {
        // Accept drops on empty space - will drop to first-level folder
        event->acceptProposedAction();
        clearSelection();
    }
}

void DraggableListWidget::dropEvent(QDropEvent *event)
{
    const QMimeData *mimeData = event->mimeData();

    if (!mimeData->hasUrls()) {
        event->ignore();
        return;
    }

    // Get the item where files were dropped (use pos() for Qt5/Qt6 compatibility)
    QListWidgetItem *item = itemAt(event->pos());

    if (item) {
        // Dropping on a specific folder item
        QString targetFolder = getTargetFolderPath(item);

        if (!targetFolder.isEmpty()) {
            // Move files to the target folder
            if (moveFilesToFolder(mimeData, targetFolder)) {
                event->acceptProposedAction();

                // Refresh the parent FloatingZone's file list
                FloatingZone *zone = qobject_cast<FloatingZone*>(parentWidget()->parentWidget());
                if (zone) {
                    zone->loadFilesFromFolder();
                }
            } else {
                event->ignore();
            }
        } else {
            event->ignore();
        }
    } else {
        // Dropping on empty space - move to the FloatingZone's root folder (first-level folder)
        FloatingZone *zone = qobject_cast<FloatingZone*>(parentWidget()->parentWidget());
        if (zone && !zone->getFolderPath().isEmpty()) {
            if (moveFilesToFolder(mimeData, zone->getFolderPath())) {
                event->acceptProposedAction();
                zone->loadFilesFromFolder();
            } else {
                event->ignore();
            }
        } else {
            event->ignore();
        }
    }
}

QString DraggableListWidget::getTargetFolderPath(QListWidgetItem *item)
{
    if (!item) {
        return QString();
    }

    QString filePath = item->data(Qt::UserRole).toString();
    QFileInfo fileInfo(filePath);

    // Only return path if it's a directory
    if (fileInfo.exists() && fileInfo.isDir()) {
        return filePath;
    }

    return QString();
}

bool DraggableListWidget::moveFilesToFolder(const QMimeData *mimeData, const QString &targetFolder)
{
    return DragDropHandler::moveFilesToFolder(mimeData, targetFolder, this);
}

// Implementation of DragDropHandler
void DragDropHandler::handleDragEnter(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void DragDropHandler::handleDrop(QDropEvent *event, FloatingZone *zone)
{
    const QMimeData *mimeData = event->mimeData();

    // Check if this is an internal drag (from this list widget)
    if (event->source() == zone->getFileList()) {
        event->ignore();
        return;
    }

    if (mimeData->hasUrls() && !zone->getFolderPath().isEmpty()) {
        QString targetFolder = zone->getFolderPath();
        
        if (moveFilesToFolder(mimeData, targetFolder, zone)) {
            event->acceptProposedAction();
            zone->refreshFileList();
        } else {
            event->ignore();
        }
    }
}

bool DragDropHandler::moveFilesToFolder(const QMimeData *mimeData, const QString &targetFolder, QWidget *parent)
{
    if (!mimeData->hasUrls() || targetFolder.isEmpty()) {
        return false;
    }

    QList<QUrl> urlList = mimeData->urls();
    QDir targetDir(targetFolder);

    if (!targetDir.exists()) {
        if (parent) {
            QMessageBox::warning(parent, QObject::tr("错误"),
                               QObject::tr("目标文件夹不存在: %1").arg(targetFolder));
        }
        return false;
    }

    int successCount = 0;
    int failCount = 0;
    QStringList failedFiles;

    for (const QUrl &url : urlList) {
        QString sourcePath = url.toLocalFile();
        QFileInfo sourceInfo(sourcePath);

        if (!sourceInfo.exists()) {
            continue;
        }

        // Skip if file is already in the target folder
        if (sourceInfo.absolutePath() == targetDir.absolutePath()) {
            continue;
        }

        QString targetPath = targetDir.absoluteFilePath(sourceInfo.fileName());

        // Handle name conflicts
        if (QFile::exists(targetPath) || QDir(targetPath).exists()) {
            QString baseName = sourceInfo.completeBaseName();
            QString suffix = sourceInfo.suffix();
            int counter = 1;

            do {
                if (sourceInfo.isDir()) {
                    targetPath = targetDir.absoluteFilePath(
                        QString("%1_%2").arg(baseName).arg(counter));
                } else {
                    targetPath = targetDir.absoluteFilePath(
                        QString("%1_%2.%3").arg(baseName).arg(counter).arg(suffix));
                }
                counter++;
            } while (QFile::exists(targetPath) || QDir(targetPath).exists());
        }

        // Try to move the file/folder
        bool success = false;
        if (sourceInfo.isDir()) {
            // For directories, use QDir::rename
            success = QDir().rename(sourcePath, targetPath);
        } else {
            // For files, try rename first (fast if same drive)
            success = QFile::rename(sourcePath, targetPath);

            // If rename fails, try copy+delete (for cross-drive moves)
            if (!success) {
                if (QFile::copy(sourcePath, targetPath)) {
                    success = QFile::remove(sourcePath);
                    if (!success) {
                        QFile::remove(targetPath); // Clean up copied file
                    }
                }
            }
        }

        if (success) {
            successCount++;
        } else {
            failCount++;
            failedFiles.append(sourceInfo.fileName());
        }
    }

    // Only show message if there were failures
    if (failCount > 0 && parent) {
        QString msg;
        if (successCount > 0) {
            msg = QObject::tr("成功移动 %1 个文件\n失败 %2 个: %3")
                .arg(successCount)
                .arg(failCount)
                .arg(failedFiles.join(", "));
        } else {
            msg = QObject::tr("移动失败: %1").arg(failedFiles.join(", "));
        }
        QMessageBox::warning(parent, QObject::tr("移动完成"), msg);
    }

    return successCount > 0;
}

