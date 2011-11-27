#include "mainwindow.h"
#include "bundlemanager.h"
#include "navigator.h"
#include "window.h"

#include <QApplication>
#include <QAction>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QToolBar>
#include <QToolButton>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    bundleManager = new BundleManager(this);
    bundleManager->readThemes("redcar-bundles/Themes");
    bundleManager->readBundles("redcar-bundles/Bundles");

    win = new Window(bundleManager);
    setCentralWidget(win);
    connect(bundleManager, SIGNAL(themeChanged(Theme)), win, SLOT(themeChanged(Theme)));

    navigator = new Navigator(this);
    connect(navigator, SIGNAL(activated(QString)), this, SLOT(visitFile(QString)));
    connect(navigator, SIGNAL(themeChange(QString)), bundleManager, SLOT(setThemeName(QString)));
    navigator->setThemeNames(bundleManager->themeNames());

    QAction* quitAction = new QAction(tr("Quit"), this);
    quitAction->setIcon(QIcon::fromTheme("application-exit"));
    quitAction->setShortcut(QKeySequence(QKeySequence::Quit));
    connect(quitAction, SIGNAL(triggered()), QApplication::instance(), SLOT(quit()));
    addAction(quitAction);

    QAction* pathFocusAction = new QAction(tr("Focus path"), this);
    pathFocusAction->setShortcut(QKeySequence(tr("Ctrl+L")));
    connect(pathFocusAction, SIGNAL(triggered()), navigator, SLOT(setFileFocus()));
    addAction(pathFocusAction);

    QAction* backAction = new QAction(tr("Back"), this);
    backAction->setIcon(QIcon::fromTheme("go-previous"));
    backAction->setShortcut(QKeySequence::Back);
    QAction* forwardAction = new QAction(tr("Forward"), this);
    forwardAction->setIcon(QIcon::fromTheme("go-next"));
    forwardAction->setShortcut(QKeySequence::Forward);

    connect(backAction, SIGNAL(triggered()), this, SLOT(historyBack()));
    connect(forwardAction, SIGNAL(triggered()), this, SLOT(historyForward()));
    connect(this, SIGNAL(historyBackAvailable(bool)), backAction, SLOT(setEnabled(bool)));
    connect(this, SIGNAL(historyForwardAvailable(bool)), forwardAction, SLOT(setEnabled(bool)));

    QMenu* menu = new QMenu(tr("Menu"));
    menu->addAction(quitAction);

    QToolButton* menuButton = new QToolButton(this);
    menuButton->setIcon(QIcon::fromTheme("document-properties"));
    menuButton->setMenu(menu);
    menuButton->setPopupMode(QToolButton::InstantPopup);

    QToolBar *tb = new QToolBar(tr("Main"), this);
    tb->addAction(backAction);
    tb->addAction(forwardAction);
    tb->addWidget(navigator);
    tb->addWidget(menuButton);
    tb->setMovable(false);
    addToolBar(tb);

    resize(800, 600);

    navigator->setFileFocus();

    historyUpdate();
}

MainWindow::~MainWindow()
{
}

void MainWindow::historyBack()
{
    historyWalk(historyBackStack, historyForwardStack);
    historyUpdate();
}

void MainWindow::historyForward()
{
    historyWalk(historyForwardStack, historyBackStack);
    historyUpdate();
}

void MainWindow::visitFile(const QString& fileName)
{
    if (win->currentFileName() != fileName) {
        if (!win->currentFileName().isEmpty()) {
            historyBackStack.push(win->currentFileName());
        }
        historyForwardStack.clear();
    }

    win->visitFile(fileName);

    historyUpdate();
}

void MainWindow::historyUpdate()
{
    emit historyBackAvailable(!historyBackStack.isEmpty());
    emit historyForwardAvailable(!historyForwardStack.isEmpty());
}

void MainWindow::historyWalk(QStack<QString>& back, QStack<QString>& forward)
{
    // Sanity check
    if (back.isEmpty()) {
        qWarning("Nothing found in history");
        return;
    }

    QString fileName = back.pop();
    forward.push(win->currentFileName());

    win->visitFile(fileName);
    navigator->setFileName(fileName);
}
