#include "editor.h"
#include "highlighter.h"

#include <QAction>
#include <QFile>
#include <QTextBlock>
#include <QKeyEvent>
#include <QToolTip>

#include <QtDebug>

EditorBlockData::EditorBlockData()
{
}

EditorBlockData::~EditorBlockData()
{
}

EditorBlockData* EditorBlockData::forBlock(QTextBlock block)
{
    if (!block.isValid())
        return 0;

    if (!block.userData()) {
        block.setUserData(new EditorBlockData);
    }
    return static_cast<EditorBlockData*>(block.userData());
}

Editor::Editor(QWidget *parent) :
    QTextEdit(parent)
{
    {
        QAction* action = new QAction(this);
        action->setShortcut(QKeySequence(tr("Ctrl+I")));
        connect(action, SIGNAL(triggered()), this, SLOT(indentLine()));
        addAction(action);
    }
    {
        QAction* action = new QAction(this);
        action->setShortcut(QKeySequence(tr("Ctrl+K")));
        connect(action, SIGNAL(triggered()), this, SLOT(killLine()));
        addAction(action);
    }
}

QString Editor::scopeForCursor(const QTextCursor& cursor) const
{
    QTextBlock block = cursor.block();
    if (!block.isValid())
        return QString();

    EditorBlockData *blockData = EditorBlockData::forBlock(cursor.block());
    QMap<QTextCursor, QString>::const_iterator it = blockData->scopes.lowerBound(cursor);
    if (it != blockData->scopes.end() && it.key().anchor() <= cursor.position()) {
        return it.value();
    }
    return QString();
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

bool Editor::isLeadingWhitespace(const QTextCursor& cursor) const
{
    int indent;
    if (currentIndent(cursor, &indent)) {
        int pos = cursor.positionInBlock();
        if (pos > 0 && pos <= indent)
            return true;
    }
    return false;
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

void Editor::doKillLine(QTextCursor cursor)
{
    // Make selection contain whole blocks
    {
        int end = cursor.selectionEnd();
        cursor.setPosition(cursor.selectionStart());
        cursor.movePosition(QTextCursor::StartOfBlock);
        cursor.setPosition(end, QTextCursor::KeepAnchor);
        if (! (cursor.hasSelection() && cursor.atBlockStart())) {
            cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
        }
    }

    cursor.removeSelectedText();
}

void Editor::indentLine()
{
    doIndent(textCursor());
}

void Editor::killLine()
{
    doKillLine(textCursor());
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
    } else if (e->key() == Qt::Key_Backspace && e->modifiers() == Qt::NoModifier) {
        QTextCursor c = textCursor();
        if (isLeadingWhitespace(c)) {
            do {
                c.deletePreviousChar();
            } while (c.positionInBlock() % 4);
        } else {
            QTextEdit::keyPressEvent(e);
        }
    } else {
        QTextEdit::keyPressEvent(e);
    }
}

bool Editor::event(QEvent *e)
{
    if (e->type() == QEvent::ToolTip) {
        QHelpEvent *he = static_cast<QHelpEvent*>(e);
        QTextCursor c = cursorForPosition(he->pos());
        if (!c.isNull()) {
            QString scope = scopeForCursor(c);
            QToolTip::showText(he->globalPos(),
                               QString("L:%1 C:%2<br/>%3")
                               .arg(c.blockNumber())
                               .arg(c.columnNumber())
                               .arg(scope));
        } else {
            QToolTip::hideText();
            he->ignore();
        }
        return true;
    } else {
        return QTextEdit::event(e);
    }
}
