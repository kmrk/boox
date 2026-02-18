#include "fileops.h"
#include <QDesktopServices>
#include <QUrl>
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QClipboard>
#include <QApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QObject>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

// ---------------------------------------------------------------------------
// Shared dark-theme style
// ---------------------------------------------------------------------------
static const char *DIALOG_STYLE =
    "QDialog {"
    "  background-color: rgba(30, 30, 30, 245);"
    "  border: 1px solid rgba(255,255,255,60);"
    "  border-radius: 8px;"
    "}"
    "QLabel {"
    "  color: rgba(255,255,255,200);"
    "  font-size: 13px;"
    "  background: transparent;"
    "}"
    "QLineEdit {"
    "  background-color: rgba(255,255,255,15);"
    "  color: white;"
    "  border: 1px solid rgba(255,255,255,60);"
    "  border-radius: 4px;"
    "  padding: 5px 8px;"
    "  font-size: 13px;"
    "}"
    "QLineEdit:focus {"
    "  border: 1px solid rgba(255,255,255,120);"
    "}"
    "QPushButton {"
    "  background-color: rgba(255,255,255,20);"
    "  color: white;"
    "  border: 1px solid rgba(255,255,255,50);"
    "  border-radius: 4px;"
    "  padding: 5px 18px;"
    "  font-size: 13px;"
    "}"
    "QPushButton:hover {"
    "  background-color: rgba(255,255,255,40);"
    "}"
    "QPushButton:pressed {"
    "  background-color: rgba(255,255,255,60);"
    "}"
    "QPushButton#btnConfirm {"
    "  background-color: rgba(80,120,200,180);"
    "  border: 1px solid rgba(80,120,200,220);"
    "}"
    "QPushButton#btnConfirm:hover {"
    "  background-color: rgba(80,120,200,230);"
    "}";

// ---------------------------------------------------------------------------
// askText: styled replacement for QInputDialog::getText
// Returns true and sets `result` on confirm; false on cancel.
// ---------------------------------------------------------------------------
static bool askText(QWidget *parent, const QString &title,
                    const QString &label, const QString &defaultValue,
                    QString &result)
{
    QDialog dlg(parent, Qt::Dialog | Qt::FramelessWindowHint);
    dlg.setStyleSheet(DIALOG_STYLE);
    dlg.setWindowTitle(title);
    dlg.setFixedWidth(300);

    QVBoxLayout *layout = new QVBoxLayout(&dlg);
    layout->setContentsMargins(20, 18, 20, 16);
    layout->setSpacing(10);

    QLabel *titleLbl = new QLabel(title, &dlg);
    titleLbl->setStyleSheet("color: white; font-size: 14px; font-weight: bold; background: transparent;");
    layout->addWidget(titleLbl);

    QLabel *lbl = new QLabel(label, &dlg);
    layout->addWidget(lbl);

    QLineEdit *edit = new QLineEdit(defaultValue, &dlg);
    edit->selectAll();
    layout->addWidget(edit);

    layout->addSpacing(4);

    QHBoxLayout *btnRow = new QHBoxLayout();
    btnRow->setSpacing(8);
    QPushButton *btnCancel  = new QPushButton(QObject::tr("取消"), &dlg);
    QPushButton *btnConfirm = new QPushButton(QObject::tr("确定"), &dlg);
    btnConfirm->setObjectName("btnConfirm");
    btnConfirm->setDefault(true);
    btnRow->addStretch();
    btnRow->addWidget(btnCancel);
    btnRow->addWidget(btnConfirm);
    layout->addLayout(btnRow);

    QObject::connect(btnCancel,  &QPushButton::clicked, &dlg, &QDialog::reject);
    QObject::connect(btnConfirm, &QPushButton::clicked, &dlg, &QDialog::accept);
    QObject::connect(edit, &QLineEdit::returnPressed,   &dlg, &QDialog::accept);

    if (dlg.exec() != QDialog::Accepted || edit->text().trimmed().isEmpty()) {
        return false;
    }
    result = edit->text().trimmed();
    return true;
}

// ---------------------------------------------------------------------------
// showWarning: styled replacement for QMessageBox::warning
// ---------------------------------------------------------------------------
static void showWarning(QWidget *parent, const QString &title, const QString &message)
{
    QDialog dlg(parent, Qt::Dialog | Qt::FramelessWindowHint);
    dlg.setStyleSheet(DIALOG_STYLE);
    dlg.setWindowTitle(title);
    dlg.setFixedWidth(300);

    QVBoxLayout *layout = new QVBoxLayout(&dlg);
    layout->setContentsMargins(20, 18, 20, 16);
    layout->setSpacing(10);

    QLabel *titleLbl = new QLabel(title, &dlg);
    titleLbl->setStyleSheet("color: rgba(255,100,100,220); font-size: 14px; font-weight: bold; background: transparent;");
    layout->addWidget(titleLbl);

    QLabel *msgLbl = new QLabel(message, &dlg);
    msgLbl->setWordWrap(true);
    layout->addWidget(msgLbl);

    layout->addSpacing(4);

    QHBoxLayout *btnRow = new QHBoxLayout();
    QPushButton *btnOk = new QPushButton(QObject::tr("确定"), &dlg);
    btnOk->setObjectName("btnConfirm");
    btnOk->setDefault(true);
    btnRow->addStretch();
    btnRow->addWidget(btnOk);
    layout->addLayout(btnRow);

    QObject::connect(btnOk, &QPushButton::clicked, &dlg, &QDialog::accept);

    dlg.exec();
}

// ---------------------------------------------------------------------------
// showConfirm: styled replacement for QMessageBox::question (Yes/No)
// Returns true if user clicked "确定".
// ---------------------------------------------------------------------------
static bool showConfirm(QWidget *parent, const QString &title, const QString &message)
{
    QDialog dlg(parent, Qt::Dialog | Qt::FramelessWindowHint);
    dlg.setStyleSheet(DIALOG_STYLE);
    dlg.setWindowTitle(title);
    dlg.setFixedWidth(300);

    QVBoxLayout *layout = new QVBoxLayout(&dlg);
    layout->setContentsMargins(20, 18, 20, 16);
    layout->setSpacing(10);

    QLabel *titleLbl = new QLabel(title, &dlg);
    titleLbl->setStyleSheet("color: white; font-size: 14px; font-weight: bold; background: transparent;");
    layout->addWidget(titleLbl);

    QLabel *msgLbl = new QLabel(message, &dlg);
    msgLbl->setWordWrap(true);
    layout->addWidget(msgLbl);

    layout->addSpacing(4);

    QHBoxLayout *btnRow = new QHBoxLayout();
    btnRow->setSpacing(8);
    QPushButton *btnCancel  = new QPushButton(QObject::tr("取消"), &dlg);
    QPushButton *btnConfirm = new QPushButton(QObject::tr("确定"), &dlg);
    btnConfirm->setObjectName("btnConfirm");
    btnConfirm->setDefault(true);
    btnRow->addStretch();
    btnRow->addWidget(btnCancel);
    btnRow->addWidget(btnConfirm);
    layout->addLayout(btnRow);

    QObject::connect(btnCancel,  &QPushButton::clicked, &dlg, &QDialog::reject);
    QObject::connect(btnConfirm, &QPushButton::clicked, &dlg, &QDialog::accept);

    return dlg.exec() == QDialog::Accepted;
}

// ---------------------------------------------------------------------------
// FileOpsHandler implementations
// ---------------------------------------------------------------------------

void FileOpsHandler::openFile(const QString &filePath, QWidget *parent)
{
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(filePath))) {
        showWarning(parent, QObject::tr("错误"),
                    QObject::tr("无法打开文件: %1").arg(filePath));
    }
}

void FileOpsHandler::deleteFile(const QString &filePath, QWidget *parent,
                                std::function<void()> onSuccess)
{
    QFileInfo fileInfo(filePath);

    if (!showConfirm(parent, QObject::tr("确认删除"),
                     QObject::tr("确定要删除 \"%1\" 吗？").arg(fileInfo.fileName()))) {
        return;
    }

    bool success = false;
    if (fileInfo.isDir()) {
        success = QDir(filePath).removeRecursively();
    } else {
        success = QFile::remove(filePath);
    }

    if (success) {
        if (onSuccess) onSuccess();
    } else {
        showWarning(parent, QObject::tr("错误"),
                    QObject::tr("无法删除: %1").arg(filePath));
    }
}

void FileOpsHandler::copyFilePath(const QString &filePath)
{
    QApplication::clipboard()->setText(filePath);
}

void FileOpsHandler::renameFolder(const QString &folderPath, const QString &currentName,
                                  QWidget *parent,
                                  std::function<void(const QString &newFolderPath)> onSuccess)
{
    QString newName;
    if (!askText(parent, QObject::tr("重命名"), QObject::tr("输入新名称:"), currentName, newName)) {
        return;
    }
    if (newName == currentName) {
        return;
    }

    QFileInfo folderInfo(folderPath);
    QString newFolderPath = folderInfo.dir().absoluteFilePath(newName);

    if (QDir(newFolderPath).exists()) {
        showWarning(parent, QObject::tr("错误"),
                    QObject::tr("文件夹 \"%1\" 已存在").arg(newName));
        return;
    }

    if (!QDir().rename(folderPath, newFolderPath)) {
        showWarning(parent, QObject::tr("错误"), QObject::tr("无法重命名文件夹"));
        return;
    }

    // Remove stale layout entry for the old path
    QString layoutFilePath = "d:/boox/.layout.json";
    QFile layoutFile(layoutFilePath);
    if (layoutFile.exists() && layoutFile.open(QIODevice::ReadOnly)) {
        QByteArray data = layoutFile.readAll();
        layoutFile.close();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            if (obj.contains(folderPath)) {
                obj.remove(folderPath);
                if (layoutFile.open(QIODevice::WriteOnly)) {
                    layoutFile.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
                    layoutFile.close();
                }
            }
        }
    }

    if (onSuccess) onSuccess(newFolderPath);
}

void FileOpsHandler::createFile(const QString &folderPath, QWidget *parent,
                                std::function<void()> onSuccess)
{
    QString name;
    if (!askText(parent, QObject::tr("新建文件"), QObject::tr("文件名:"),
                 QObject::tr("新建文件.txt"), name)) {
        return;
    }

    QString filePath = QDir(folderPath).absoluteFilePath(name);

    if (QFile::exists(filePath)) {
        showWarning(parent, QObject::tr("错误"),
                    QObject::tr("文件 \"%1\" 已存在").arg(name));
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        showWarning(parent, QObject::tr("错误"),
                    QObject::tr("无法创建文件: %1").arg(name));
        return;
    }
    file.close();

    if (onSuccess) onSuccess();
}

void FileOpsHandler::createFolder(const QString &folderPath, QWidget *parent,
                                  std::function<void()> onSuccess)
{
    QString name;
    if (!askText(parent, QObject::tr("新建文件夹"), QObject::tr("文件夹名:"),
                 QObject::tr("新建文件夹"), name)) {
        return;
    }

    QString newPath = QDir(folderPath).absoluteFilePath(name);

    if (QDir(newPath).exists()) {
        showWarning(parent, QObject::tr("错误"),
                    QObject::tr("文件夹 \"%1\" 已存在").arg(name));
        return;
    }

    if (!QDir().mkdir(newPath)) {
        showWarning(parent, QObject::tr("错误"),
                    QObject::tr("无法创建文件夹: %1").arg(name));
        return;
    }

    if (onSuccess) onSuccess();
}
