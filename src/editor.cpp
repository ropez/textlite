#include "editor.h"

#include <QFile>

Editor::Editor(QWidget *parent) :
    QTextEdit(parent)
{
    setReadOnly(true);
    setWordWrapMode(QTextOption::NoWrap);
    setFontFamily("DejaVu Sans Mono");
}
