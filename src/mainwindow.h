#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui/QMainWindow>

class Navigator;
class BundleManager;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Navigator* navigator;
    BundleManager* bundleManager;
};

#endif // MAINWINDOW_H
