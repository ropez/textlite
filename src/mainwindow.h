#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui/QMainWindow>
#include <QtCore/QStack>

class Window;
class Navigator;
class BundleManager;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void back();
    void forward();

    void setFileName(const QString& fileName);

private:
    void navigate(QStack<QString>& from, QStack<QString>& to);

private:
    Window* win;
    Navigator* navigator;
    BundleManager* bundleManager;

    QStack<QString> backStack;
    QStack<QString> forwardStack;
};

#endif // MAINWINDOW_H
