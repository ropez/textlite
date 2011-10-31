#include "window.h"
#include "navigator.h"
#include "editor.h"
#include "bundlemanager.h"
#include "highlighter.h" // XXX thememanager.h

#include <QAction>
#include <QLayout>
#include <QTimer>
#include <QFileInfo>
#include <QFileSystemWatcher>

#include <QtDebug>

Window::Window(QWidget *parent) :
    QWidget(parent)
{
    QVBoxLayout *vl = new QVBoxLayout(this);
    navigator = new Navigator(this);
    editor = new Editor(this);
    vl->addWidget(navigator);
    vl->addWidget(editor);

    editor->setReadOnly(true);
    editor->setWordWrapMode(QTextOption::NoWrap);

    bundleManager = new BundleManager("redcar-bundles/Themes", this);
    bundleManager->readBundles("redcar-bundles/Bundles");

    // XXX
    bundleManager->readThemeFile("Espresso.tmTheme");

    watcher = new QFileSystemWatcher(this);
    reloadTimer = new QTimer(this);
    reloadTimer->setSingleShot(true);

    connect(navigator, SIGNAL(activated(QString)), this, SLOT(setFileName(QString)));
    connect(navigator, SIGNAL(themeChange(QString)), bundleManager, SLOT(readThemeFile(QString)));
    connect(editor, SIGNAL(textChanged()), this, SLOT(saveFile()));
    connect(watcher, SIGNAL(fileChanged(QString)), this, SLOT(readFileLater(QString)));
    connect(reloadTimer, SIGNAL(timeout()), this, SLOT(readPendingFiles()));

    QAction* navigate = new QAction(this);
    navigate->setShortcut(QKeySequence(tr("Ctrl+L")));
    connect(navigate, SIGNAL(triggered()), navigator, SLOT(setFileFocus()));
    addAction(navigate);
}

void Window::setFileName(const QString &name)
{
    this->filename.clear();

    if (documents.contains(name)) {
        editor->setDocument(documents.value(name));
    } else {
        editor->setDocument(new QTextDocument(this));
        documents.insert(name, editor->document());

        QFont font;
        font.setFamily("DejaVu Sans Mono");
        editor->document()->setDefaultFont(font);

        readFile(name);

        QFileInfo info(name);
        bundleManager->getHighlighterForExtension(info.completeSuffix(), editor->document());
    }

    this->filename = name;
    editor->setReadOnly(false);
    editor->setFocus();
}

void Window::saveFile()
{
    if (filename.isNull())
        return;

    watcher->removePath(filename);

    QFile file(filename);

    if (!file.open(QFile::WriteOnly)) {
        qWarning("Permission denied");
        return;
    }

    file.write(editor->document()->toPlainText().toUtf8());
    file.close();

    watcher->addPath(filename);
}

void Window::readFile(const QString& name)
{
    if (documents.contains(name)) {
        QTextDocument* doc = documents.value(name);

        QFile file(name);
        if (file.open(QFile::ReadOnly)) {
            doc->setPlainText(QString::fromUtf8(file.readAll()));
        } else {
            qWarning() << "File not found:" << name;
        }
    }
    watcher->addPath(name);
}

void Window::readFileLater(const QString& name)
{
    if (!reloadNames.contains(name))
        reloadNames.enqueue(name);
    reloadTimer->start(1000);
}

void Window::readPendingFiles()
{
    QString fn = this->filename;
    this->filename.clear();
    while (!reloadNames.isEmpty()) {
        QString name = reloadNames.dequeue();
        readFile(name);
    }
    this->filename = fn;
}
