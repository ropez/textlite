#ifndef WINDOW_H
#define WINDOW_H

#include <QWidget>

class Navigator;
class Editor;
class BundleManager;

class Window : public QWidget
{
    Q_OBJECT
public:
    explicit Window(QWidget *parent = 0);

private slots:
    void setFileName(const QString& fileName);

private:
    Navigator* navigator;
    Editor* editor;
    BundleManager* bundleManager;
};

#endif // WINDOW_H
