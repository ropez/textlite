#include "mainwindow.h"
#include "bundlemanager.h"
#include "navigator.h"
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
    bundleManager = new BundleManager("redcar-bundles/Themes", this);
    bundleManager->readBundles("redcar-bundles/Bundles");

    // XXX
    bundleManager->readThemeFile("Espresso.tmTheme");

    win = new Window(bundleManager);
    setCentralWidget(win);
    connect(bundleManager, SIGNAL(themeChanged(Theme)), win, SLOT(themeChanged(Theme)));

    navigator = new Navigator(this);
    connect(navigator, SIGNAL(activated(QString)), this, SLOT(setFileName(QString)));
    connect(navigator, SIGNAL(themeChange(QString)), bundleManager, SLOT(readThemeFile(QString)));

    QAction* quit = new QAction(this);
    quit->setIcon(QIcon::fromTheme("application-exit"));
    quit->setShortcut(QKeySequence(QKeySequence::Quit));
    connect(quit, SIGNAL(triggered()), QApplication::instance(), SLOT(quit()));
    addAction(quit);

    QAction* navigate = new QAction(this);
    navigate->setShortcut(QKeySequence(tr("Ctrl+L")));
    connect(navigate, SIGNAL(triggered()), navigator, SLOT(setFileFocus()));
    addAction(navigate);

    QAction* prev = new QAction(this);
    prev->setIcon(QIcon::fromTheme("go-previous"));
    QAction* next = new QAction(this);
    next->setIcon(QIcon::fromTheme("go-next"));

    connect(prev, SIGNAL(triggered()), this, SLOT(back()));
    connect(next, SIGNAL(triggered()), this, SLOT(forward()));

    QToolBar *tb = new QToolBar(tr("Main"), this);
    tb->addAction(prev);
    tb->addAction(next);

//    QLineEdit* edit = new QLineEdit(this);
//    tb->addSeparator();
//    tb->addWidget(new QLabel(tr("File:")));
//    tb->addWidget(edit);

    tb->addWidget(navigator);

    tb->setMovable(false);
    addToolBar(tb);
    resize(800, 600);

    navigator->setFileFocus();
}

MainWindow::~MainWindow()
{
}

void MainWindow::back()
{
    navigate(backStack, forwardStack);
}

void MainWindow::forward()
{
    navigate(forwardStack, backStack);
}

void MainWindow::setFileName(const QString& fileName)
{
    backStack.push(fileName);
    forwardStack.clear();
    win->setFileName(fileName);
}

void MainWindow::navigate(QStack<QString>& from, QStack<QString>& to)
{
    QString fileName;
    do {
        if (from.isEmpty())
            return;
        fileName = from.pop();
        to.push(fileName);
    } while (fileName == navigator->fileName());

    win->setFileName(fileName);
    navigator->setFileName(fileName);
}
