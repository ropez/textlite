#include "editor.h"

#include <QFile>

Editor::Editor(QWidget *parent) :
    QTextEdit(parent)
{
    setReadOnly(true);
    setWordWrapMode(QTextOption::NoWrap);
    setFontFamily("DejaVu Sans Mono");

    connect(this, SIGNAL(textChanged()), this, SLOT(saveFile()));
}

void Editor::setFileName(const QString &name)
{
    this->filename.clear();

    if (documents.contains(name)) {
        setDocument(documents.value(name));
    } else {
        setDocument(new QTextDocument(this));
        documents.insert(name, document());

        QFile file(name);
        if (file.open(QFile::ReadOnly)) {
            document()->setPlainText(file.readAll());
        } else {
            qWarning("File not found");
        }
    }

    this->filename = name;
    setReadOnly(false);
    setFocus();
}

void Editor::saveFile()
{
    if (filename.isNull())
        return;

    QFile file(filename);

    if (!file.open(QFile::WriteOnly)) {
        qWarning("Permission denied");
        return;
    }

    file.write(toPlainText().toUtf8());
}
