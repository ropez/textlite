#include "window.h"
#include "navigator.h"
#include "editor.h"
#include "bundlemanager.h"

#include <QAction>
#include <QLayout>
#include <QFileInfo>

Window::Window(QWidget *parent) :
    QWidget(parent)
{
    QVBoxLayout *vl = new QVBoxLayout(this);
    navigator = new Navigator(this);
    editor = new Editor(this);
    vl->addWidget(navigator);
    vl->addWidget(editor);

    bundleManager = new BundleManager(this);
    bundleManager->readBundles("redcar-bundles/Bundles");

    connect(navigator, SIGNAL(activated(QString)), this, SLOT(setFileName(QString)));

    QAction* navigate = new QAction(this);
    navigate->setShortcut(QKeySequence(tr("Ctrl+L")));
    connect(navigate, SIGNAL(triggered()), navigator, SLOT(setFileFocus()));
    addAction(navigate);
}

void Window::setFileName(const QString &fileName)
{
    QFileInfo info(fileName);
    editor->setFileName(fileName);
    Bundle* bundle = bundleManager->getBundleForExtension(info.completeSuffix());
    bundle->createHighlighter(editor->document());
    delete bundle;
}
