#ifndef FILEOPS_H
#define FILEOPS_H

#include <QString>
#include <functional>

class QWidget;

// Handles file system operations: open, delete, rename folder
// All methods are static; UI feedback (dialogs) uses parentWidget
class FileOpsHandler
{
public:
    // Open file or folder with system default application
    static void openFile(const QString &filePath, QWidget *parent = nullptr);

    // Delete file or folder (with confirmation dialog)
    // onSuccess is called after successful deletion
    static void deleteFile(const QString &filePath, QWidget *parent = nullptr,
                           std::function<void()> onSuccess = nullptr);

    // Copy file path to clipboard
    static void copyFilePath(const QString &filePath);

    // Rename the folder backing a zone
    // onSuccess(newFolderPath) is called after successful rename
    static void renameFolder(const QString &folderPath, const QString &currentName,
                             QWidget *parent = nullptr,
                             std::function<void(const QString &newFolderPath)> onSuccess = nullptr);

    // Create a new empty file inside folderPath (prompts for name)
    // onSuccess is called after successful creation
    static void createFile(const QString &folderPath, QWidget *parent = nullptr,
                           std::function<void()> onSuccess = nullptr);

    // Create a new subfolder inside folderPath (prompts for name)
    // onSuccess is called after successful creation
    static void createFolder(const QString &folderPath, QWidget *parent = nullptr,
                             std::function<void()> onSuccess = nullptr);
};

#endif // FILEOPS_H
