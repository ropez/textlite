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

void Editor::doIndent(QTextCursor c)
{
    c.beginEditBlock();
    QRegExp exp("^(\\s*)((?=\\S)|$)");
    c.movePosition(QTextCursor::StartOfBlock);
    QTextBlock block = c.block();
    if (exp.indexIn(block.text()) != -1) {
        c.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, exp.matchedLength());
        c.removeSelectedText();
    }
    while ((block = block.previous()).isValid()) {
        if (exp.indexIn(block.text()) != -1) {
            c.insertText(QString(" ").repeated(exp.matchedLength()));
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
