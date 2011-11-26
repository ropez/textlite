#include "mainwindow.h"
#include "window.h"

#include <QApplication>
#include <QAction>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QToolBar>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    Window* win = new Window();
    setCentralWidget(win);

    QAction* quit = new QAction(this);
    quit->setIcon(QIcon::fromTheme("application-exit"));
    quit->setShortcut(QKeySequence(QKeySequence::Quit));
    connect(quit, SIGNAL(triggered()), QApplication::instance(), SLOT(quit()));
    addAction(quit);

    QAction* prev = new QAction(this);
    prev->setIcon(QIcon::fromTheme("go-previous"));
    QAction* next = new QAction(this);
    next->setIcon(QIcon::fromTheme("go-next"));

    QToolBar *tb = new QToolBar(tr("Main"), this);
    tb->addAction(prev);
    tb->addAction(next);

//    QLineEdit* edit = new QLineEdit(this);
//    tb->addSeparator();
//    tb->addWidget(new QLabel(tr("File:")));
//    tb->addWidget(edit);

    tb->setMovable(false);
    addToolBar(tb);
    resize(800, 600);
}

MainWindow::~MainWindow()
{
}
