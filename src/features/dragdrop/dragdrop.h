#ifndef DRAGDROP_H
#define DRAGDROP_H

#include <QListWidget>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>

class QWidget;
class FloatingZone;

// Custom QListWidget to support dragging files out and dropping files onto folders
class DraggableListWidget : public QListWidget
{
    Q_OBJECT
public:
    explicit DraggableListWidget(QWidget *parent = nullptr);

protected:
    QMimeData* mimeData(const QList<QListWidgetItem*> items) const override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    QString getTargetFolderPath(QListWidgetItem *item);
    bool moveFilesToFolder(const QMimeData *mimeData, const QString &targetFolder);
};

// Drag and drop handler for FloatingZone widget
class DragDropHandler
{
public:
    static void handleDragEnter(QDragEnterEvent *event);
    static void handleDrop(QDropEvent *event, FloatingZone *zone);
    static bool moveFilesToFolder(const QMimeData *mimeData, const QString &targetFolder, QWidget *parent = nullptr);
};

#endif // DRAGDROP_H

