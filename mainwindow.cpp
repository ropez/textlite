#include "mainwindow.h"
#include "window.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    Window* win = new Window();
    setCentralWidget(win);
}

MainWindow::~MainWindow()
{
}
