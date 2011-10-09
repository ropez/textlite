#include "editor.h"

#include <QFile>

Editor::Editor(QWidget *parent) :
    QTextEdit(parent)
{
    setEnabled(false);
    setFontFamily("DejaVu Sans Mono");
}

void Editor::setFileName(const QString &name)
{
    QFile file(name);

    if (!file.open(QFile::ReadOnly)) {
        qWarning("File not found");
        return;
    }

    setPlainText(file.readAll());
    setEnabled(true);
    setFocus();
}
