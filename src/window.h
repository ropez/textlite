#ifndef WINDOW_H
#define WINDOW_H

#include <QWidget>
#include <QMap>
#include <QQueue>

class QTextDocument;
class QFileSystemWatcher;
class QTimer;

class Navigator;
class Editor;
class BundleManager;

class Window : public QWidget
{
    Q_OBJECT
public:
    explicit Window(QWidget *parent = 0);

private slots:
    void setFileName(const QString& name);
    void saveFile();
    void readFile(const QString& name);
    void readFileLater(const QString& name);
    void readPendingFiles();

private:
    Navigator* navigator;
    Editor* editor;
    BundleManager* bundleManager;

    QString filename;
    QMap<QString, QTextDocument*> documents;

    QFileSystemWatcher* watcher;
    QTimer* reloadTimer;
    QQueue<QString> reloadNames;
};

#endif // WINDOW_H
