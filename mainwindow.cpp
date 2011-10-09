#include "mainwindow.h"
#include "window.h"

#include <QApplication>
#include <QAction>
#include <QKeySequence>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    Window* win = new Window();
    setCentralWidget(win);

    QAction* quit = new QAction(this);
    quit->setShortcut(QKeySequence(QKeySequence::Quit));
    connect(quit, SIGNAL(triggered()), QApplication::instance(), SLOT(quit()));
    addAction(quit);
}

MainWindow::~MainWindow()
{
}
