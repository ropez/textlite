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
    QMap<QTextCursor, QStringList>::const_iterator it = blockData->scopes.lowerBound(cursor);
    if (it != blockData->scopes.end() && it.key().anchor() <= cursor.position()) {
        return it.value().join("\n");
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

void Editor::doIndent(QTextCursor cursor)
{
    cursor.beginEditBlock();
    cursor.movePosition(QTextCursor::StartOfBlock);
    int indent = -1;
    if (currentIndent(cursor, &indent)) {
        cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, indent);
        cursor.removeSelectedText();
    }
    QTextCursor previous = cursor;
    while (previous.movePosition(QTextCursor::PreviousBlock)) {
        if (currentIndent(previous, &indent)) {
            cursor.insertText(QString(" ").repeated(indent));
            break;
        }
    }
    cursor.endEditBlock();
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
        QTextCursor cursor = textCursor();
        do {
            cursor.insertText(" ");
        } while (cursor.positionInBlock() % 4);
    } else if (e->key() == Qt::Key_Backspace && e->modifiers() == Qt::NoModifier) {
        QTextCursor cursor = textCursor();
        if (isLeadingWhitespace(cursor)) {
            do {
                cursor.deletePreviousChar();
            } while (cursor.positionInBlock() % 4);
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
        QHelpEvent *helpEvent = static_cast<QHelpEvent*>(e);
        QTextCursor cursor = cursorForPosition(helpEvent->pos());
        if (!cursor.isNull()) {
            QString scope = scopeForCursor(cursor);
            QToolTip::showText(helpEvent->globalPos(),
                               QString("<pre>L:%1 C:%2\n%3</pre>")
                               .arg(cursor.blockNumber())
                               .arg(cursor.columnNumber())
                               .arg(scope));
        } else {
            QToolTip::hideText();
            helpEvent->ignore();
        }
        return true;
    } else {
        return QTextEdit::event(e);
    }
}
