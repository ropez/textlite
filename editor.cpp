#include "editor.h"

#include <QFile>

Editor::Editor(QWidget *parent) :
    QTextEdit(parent)
{
    setEnabled(false);
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
    this->filename = name;
    setPlainText(file.readAll());
    setEnabled(true);
    setFocus();
}

void Editor::saveFile()
{
    QFile file(filename);

    if (!file.open(QFile::WriteOnly)) {
        qWarning("Permission denied");
        return;
    }

    file.write(toPlainText().toUtf8());
}
