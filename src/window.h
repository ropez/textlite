#ifndef WINDOW_H
#define WINDOW_H

#include <QtGui/QWidget>
#include <QtCore/QMap>
#include <QtCore/QQueue>

class QTextDocument;
class QTextCursor;
class QFileSystemWatcher;
class QLineEdit;
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

    QString currentFileName() const;

public slots:
    void find();
    void findNext();
    void findPrevious();

    void visitFile(const QString& name);

private slots:
    void saveFile();
    bool readFile(const QString& name, QTextDocument* document);
    void readFileLater(const QString& name);
    void readPendingFiles();

private slots:
    void themeChanged(const Theme& theme);

private:
    Editor* editor;

    QLineEdit* searchField;

    BundleManager* bundleManager;

    QString filename;
    QMap<QString, QTextDocument*> documents;
    QMap<QString, QTextCursor> cursors;

    QFileSystemWatcher* watcher;
    QTimer* reloadTimer;
    QQueue<QString> reloadNames;
};

#endif // WINDOW_H
