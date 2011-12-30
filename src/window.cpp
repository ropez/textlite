#include "window.h"
#include "navigator.h"
#include "editor.h"
#include "bundlemanager.h"
#include "theme.h"

#include <QAction>
#include <QLayout>
#include <QTimer>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QLineEdit>

#include <QtDebug>

Window::Window(BundleManager* bman, QWidget *parent) :
    QWidget(parent),
    bundleManager(bman)
{
    QVBoxLayout *vl = new QVBoxLayout(this);
    editor = new Editor(this);
    searchField = new QLineEdit(this);
    vl->addWidget(editor);
    vl->addWidget(searchField);
    vl->setMargin(0);
    vl->setSpacing(0);

    editor->setReadOnly(true);
    editor->setWordWrapMode(QTextOption::NoWrap);

    watcher = new QFileSystemWatcher(this);
    saveTimer = new QTimer(this);
    saveTimer->setSingleShot(true);
    reloadTimer = new QTimer(this);
    reloadTimer->setSingleShot(true);

    {
        QAction* action = new QAction("Focus editor", this);
        action->setShortcut(QKeySequence(tr("Esc")));
        addAction(action);
        connect(action, SIGNAL(triggered()), editor, SLOT(setFocus()));
    }
    {
        QAction* action = new QAction("Find", this);
        action->setShortcut(QKeySequence::Find);
        addAction(action);
        connect(action, SIGNAL(triggered()), this, SLOT(find()));
        connect(searchField, SIGNAL(returnPressed()), this, SLOT(findNext()));
    }
    {
        QAction* action = new QAction("Find next", this);
        action->setShortcut(QKeySequence::FindNext);
        addAction(action);
        connect(action, SIGNAL(triggered()), this, SLOT(findNext()));
    }
    {
        QAction* action = new QAction("Find previous", this);
        action->setShortcut(QKeySequence::FindPrevious);
        addAction(action);
        connect(action, SIGNAL(triggered()), this, SLOT(findPrevious()));
    }

    connect(editor, SIGNAL(textChanged()), this, SLOT(saveFileLater()));
    connect(watcher, SIGNAL(fileChanged(QString)), this, SLOT(readFileLater(QString)));
    connect(saveTimer, SIGNAL(timeout()), this, SLOT(savePendingFiles()));
    connect(reloadTimer, SIGNAL(timeout()), this, SLOT(readPendingFiles()));

    QFont font;
    font.setFamily("DejaVu Sans Mono");
    editor->setFont(font);
}

Window::~Window()
{
    saveTimer->stop();
    reloadTimer->stop();
    savePendingFiles();
}

QString Window::currentFileName() const
{
    return filename;
}

void Window::find()
{
    QTextCursor cursor = editor->textCursor();
    if (!cursor.hasSelection()) {
        cursor.select(QTextCursor::WordUnderCursor);
    } else if (cursor.document()->findBlock(cursor.selectionStart()) !=
               cursor.document()->findBlock(cursor.selectionEnd())) {
        // TODO: Find in selection
        cursor.clearSelection();
    }
    searchField->setText(cursor.selectedText());
    searchField->selectAll();
    searchField->setFocus();
}

void Window::findNext()
{
    editor->setFocus();
    editor->findMore(searchField->text());
}

void Window::findPrevious()
{
    editor->setFocus();
    editor->findMore(searchField->text(), QTextDocument::FindBackward);
}

void Window::visitFile(const QString &name)
{
    // Store cursor for current document
    if (!this->filename.isEmpty()){
        cursors[this->filename] = editor->textCursor();
    }

    // Disable auto-save while loading
    this->filename.clear();

    // Create document and load file
    if (!documents.contains(name)) {
        QTextDocument *doc = new QTextDocument(this);
        doc->setDefaultFont(editor->font());

        if (!readFile(name, doc)) {
            delete doc;
            return;
        }

        documents.insert(name, doc);
        cursors.insert(name, QTextCursor(doc));

        QFileInfo info(name);
        bundleManager->getHighlighterForExtension(info.completeSuffix(), doc);
    }

    // Bring to front, restore cursor
    editor->setDocument(documents.value(name));
    editor->setTextCursor(cursors.value(name));

    // Apparently, we need to repeat tab stop width when changing documents
    editor->setTabStopWidth(QFontMetrics(editor->font()).width(' ') * 4);

    // Enable auto-save
    this->filename = name;

    // Start editing
    editor->setReadOnly(false);
    editor->setFocus();
}

void Window::saveFile(const QString &name, QTextDocument* document)
{
    watcher->removePath(name);

    qDebug() << "save" << name;

    QFile file(name);

    if (!file.open(QFile::WriteOnly)) {
        qWarning("Permission denied");
        return;
    }

    file.write(document->toPlainText().toUtf8());
    file.close();

    watcher->addPath(name);
}

void Window::saveFileLater()
{
    if (filename.isNull())
        return;

    if (!saveNames.contains(filename))
        saveNames.enqueue(filename);

    saveTimer->start(1000);
}

void Window::savePendingFiles()
{
    while (!saveNames.isEmpty()) {
        QString name = saveNames.dequeue();
        Q_ASSERT(documents.contains(name));
        saveFile(name, documents.value(name));
    }
}

bool Window::readFile(const QString& name, QTextDocument *document)
{
    QFile file(name);
    if (file.open(QFile::ReadOnly)) {
        for (int i = 0; i < document->blockCount(); i++) {
            EditorBlockData::forBlock(document->findBlockByNumber(i))->scopes.clear();
        }
        document->setPlainText(QString::fromUtf8(file.readAll()));
    } else {
        qWarning() << "File not found:" << name;
        return false;
    }
    watcher->addPath(name);
    return true;
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
        Q_ASSERT(documents.contains(name));
        readFile(name, documents.value(name));
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
