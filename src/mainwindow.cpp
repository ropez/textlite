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

    Window* win = new Window(bundleManager);
    setCentralWidget(win);
    connect(bundleManager, SIGNAL(themeChanged(Theme)), win, SLOT(themeChanged(Theme)));

    navigator = new Navigator(this);
    connect(navigator, SIGNAL(activated(QString)), win, SLOT(setFileName(QString)));
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
