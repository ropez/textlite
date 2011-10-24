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

    editor->setReadOnly(true);
    editor->setWordWrapMode(QTextOption::NoWrap);

    bundleManager = new BundleManager(this);
    bundleManager->readBundles("redcar-bundles/Bundles");

    connect(navigator, SIGNAL(activated(QString)), this, SLOT(setFileName(QString)));
    connect(editor, SIGNAL(textChanged()), this, SLOT(saveFile()));

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

        QFile file(name);
        if (file.open(QFile::ReadOnly)) {
            editor->document()->setPlainText(file.readAll());
        } else {
            qWarning("File not found");
        }

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

    QFile file(filename);

    if (!file.open(QFile::WriteOnly)) {
        qWarning("Permission denied");
        return;
    }

    file.write(editor->document()->toPlainText().toUtf8());
}
