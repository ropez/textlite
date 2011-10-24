#ifndef WINDOW_H
#define WINDOW_H

#include <QWidget>
#include <QMap>

class QTextDocument;

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

private:
    Navigator* navigator;
    Editor* editor;
    BundleManager* bundleManager;

    QString filename;
    QMap<QString, QTextDocument*> documents;
};

#endif // WINDOW_H
