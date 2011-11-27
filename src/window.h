#ifndef WINDOW_H
#define WINDOW_H

#include <QtGui/QWidget>
#include <QtCore/QMap>
#include <QtCore/QQueue>

class QTextDocument;
class QFileSystemWatcher;
class QTimer;

class Theme;
class Editor;
class BundleManager;

class Window : public QWidget
{
    Q_OBJECT
public:
    explicit Window(BundleManager* bman, QWidget *parent = 0);
    ~Window();

public slots:
    void visitFile(const QString& name);

private slots:
    void saveFile();
    void readFile(const QString& name);
    void readFileLater(const QString& name);
    void readPendingFiles();

private slots:
    void themeChanged(const Theme& theme);

private:
    Editor* editor;
    BundleManager* bundleManager;

    QString filename;
    QMap<QString, QTextDocument*> documents;

    QFileSystemWatcher* watcher;
    QTimer* reloadTimer;
    QQueue<QString> reloadNames;
};

#endif // WINDOW_H
