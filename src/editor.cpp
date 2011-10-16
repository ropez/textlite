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
    QFile file(name);

    if (!file.open(QFile::ReadOnly)) {
        qWarning("File not found");
        return;
    }
    this->filename.clear();
    setPlainText(file.readAll());
    setReadOnly(false);
    setFocus();
    this->filename = name;
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
