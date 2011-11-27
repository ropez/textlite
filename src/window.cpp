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

Window::Window(BundleManager* bman, QWidget *parent) :
    QWidget(parent),
    bundleManager(bman)
{
    QVBoxLayout *vl = new QVBoxLayout(this);
    editor = new Editor(this);
    vl->addWidget(editor);
    vl->setMargin(0);

    editor->setReadOnly(true);
    editor->setWordWrapMode(QTextOption::NoWrap);

    watcher = new QFileSystemWatcher(this);
    reloadTimer = new QTimer(this);
    reloadTimer->setSingleShot(true);

    connect(editor, SIGNAL(textChanged()), this, SLOT(saveFile()));
    connect(watcher, SIGNAL(fileChanged(QString)), this, SLOT(readFileLater(QString)));
    connect(reloadTimer, SIGNAL(timeout()), this, SLOT(readPendingFiles()));
}

Window::~Window()
{
}

QString Window::currentFileName() const
{
    return filename;
}

void Window::visitFile(const QString &name)
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

        editor->setTabStopWidth(QFontMetrics(font).width(' ') * 4);

        readFile(name);
        editor->setTextCursor(QTextCursor(editor->document()));

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

void Window::themeChanged(const Theme& theme)
{
    QTextCharFormat baseFormat = theme.format("");

    QPalette p;
    p.setBrush(QPalette::Base, baseFormat.background());
    p.setBrush(QPalette::Foreground, baseFormat.foreground());
    p.setBrush(QPalette::Text, baseFormat.brushProperty(QTextFormat::UserProperty));
    editor->setPalette(p);
}
