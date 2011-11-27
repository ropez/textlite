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
    connect(navigator, SIGNAL(activated(QString)), this, SLOT(visitFile(QString)));
    connect(navigator, SIGNAL(themeChange(QString)), bundleManager, SLOT(readThemeFile(QString)));

    QAction* quitAction = new QAction(this);
    quitAction->setIcon(QIcon::fromTheme("application-exit"));
    quitAction->setShortcut(QKeySequence(QKeySequence::Quit));
    connect(quitAction, SIGNAL(triggered()), QApplication::instance(), SLOT(quit()));
    addAction(quitAction);

    QAction* pathFocusAction = new QAction(this);
    pathFocusAction->setShortcut(QKeySequence(tr("Ctrl+L")));
    connect(pathFocusAction, SIGNAL(triggered()), navigator, SLOT(setFileFocus()));
    addAction(pathFocusAction);

    QAction* backAction = new QAction(this);
    backAction->setIcon(QIcon::fromTheme("go-previous"));
    QAction* forwardAction = new QAction(this);
    forwardAction->setIcon(QIcon::fromTheme("go-next"));

    connect(backAction, SIGNAL(triggered()), this, SLOT(historyBack()));
    connect(forwardAction, SIGNAL(triggered()), this, SLOT(historyForward()));

    QToolBar *tb = new QToolBar(tr("Main"), this);
    tb->addAction(backAction);
    tb->addAction(forwardAction);
    tb->addWidget(navigator);
    tb->setMovable(false);
    addToolBar(tb);

    resize(800, 600);

    navigator->setFileFocus();
}

MainWindow::~MainWindow()
{
}

void MainWindow::historyBack()
{
    historyWalk(historyBackStack, historyForwardStack);
}

void MainWindow::historyForward()
{
    historyWalk(historyForwardStack, historyBackStack);
}

void MainWindow::visitFile(const QString& fileName)
{
    historyBackStack.push(fileName);
    historyForwardStack.clear();
    win->visitFile(fileName);
}

void MainWindow::historyWalk(QStack<QString>& back, QStack<QString>& forward)
{
    QString fileName;
    do {
        if (back.isEmpty())
            return;
        fileName = back.pop();
        forward.push(fileName);
    } while (fileName == navigator->fileName());

    win->visitFile(fileName);
    navigator->setFileName(fileName);
}
