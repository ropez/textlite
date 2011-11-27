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
    void historyBack();
    void historyForward();

    void visitFile(const QString& fileName);

signals:
    void historyBackAvailable(bool yes);
    void historyForwardAvailable(bool yes);

private:
    void historyWalk(QStack<QString>& back, QStack<QString>& forward);

private:
    Window* win;
    Navigator* navigator;
    BundleManager* bundleManager;

    QStack<QString> historyBackStack;
    QStack<QString> historyForwardStack;
};

#endif // MAINWINDOW_H
