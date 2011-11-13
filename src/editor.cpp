#include "editor.h"

#include <QAction>
#include <QFile>
#include <QTextBlock>
#include <QKeyEvent>

#include <QtDebug>

Editor::Editor(QWidget *parent) :
    QTextEdit(parent)
{
    QAction* action = new QAction(this);
    action->setShortcut(QKeySequence(tr("Ctrl+I")));
    connect(action, SIGNAL(triggered()), this, SLOT(indentLine()));
    addAction(action);
}

bool Editor::currentIndent(const QTextCursor& cursor, int* indent) const
{
    QRegExp exp("^(\\s*)((?=\\S)|$)");
    if (exp.indexIn(cursor.block().text()) != -1) {
        *indent = exp.matchedLength();
        return true;
    } else {
        *indent = -1;
        return false;
    }
}

void Editor::doIndent(QTextCursor c)
{
    c.beginEditBlock();
    c.movePosition(QTextCursor::StartOfBlock);
    int indent = -1;
    if (currentIndent(c, &indent)) {
        c.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, indent);
        c.removeSelectedText();
    }
    QTextCursor p = c;
    while (p.movePosition(QTextCursor::PreviousBlock)) {
        if (currentIndent(p, &indent)) {
            c.insertText(QString(" ").repeated(indent));
            break;
        }
    }
    c.endEditBlock();
}

void Editor::indentLine()
{
    doIndent(textCursor());
}

void Editor::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Return) {
        QTextEdit::keyPressEvent(e);
        doIndent(textCursor());
    } else if (e->key() == Qt::Key_Tab) {
        QTextCursor c = textCursor();
        do {
            c.insertText(" ");
        } while (c.positionInBlock() % 4);
    } else {
        QTextEdit::keyPressEvent(e);
    }
}
