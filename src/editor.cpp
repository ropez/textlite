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
        action->setShortcut(QKeySequence(tr("Ctrl+]")));
        connect(action, SIGNAL(triggered()), this, SLOT(increaseIndent()));
        addAction(action);
    }
    {
        QAction* action = new QAction(this);
        action->setShortcut(QKeySequence(tr("Ctrl+[")));
        connect(action, SIGNAL(triggered()), this, SLOT(decreaseIndent()));
        addAction(action);
    }
    {
        QAction* action = new QAction(this);
        action->setShortcut(QKeySequence(tr("Ctrl+K")));
        connect(action, SIGNAL(triggered()), this, SLOT(killLine()));
        addAction(action);
    }
    {
        QAction* action = new QAction(this);
        action->setShortcut(QKeySequence(tr("Ctrl+Shift+Up")));
        connect(action, SIGNAL(triggered()), this, SLOT(moveRegionUp()));
        addAction(action);
    }
    {
        QAction* action = new QAction(this);
        action->setShortcut(QKeySequence(tr("Ctrl+Shift+Down")));
        connect(action, SIGNAL(triggered()), this, SLOT(moveRegionDown()));
        addAction(action);
    }
    {
        QAction* action = new QAction(this);
        action->setShortcut(QKeySequence(tr("Ctrl+Shift+B")));
        connect(action, SIGNAL(triggered()), this, SLOT(selectBlocks()));
        addAction(action);
    }
    connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(highlightMatching()));
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

void Editor::doSelectBlocks(QTextCursor& cursor)
{
    int end = cursor.selectionEnd();
    cursor.setPosition(cursor.selectionStart());
    cursor.movePosition(QTextCursor::StartOfBlock);
    cursor.setPosition(end, QTextCursor::KeepAnchor);
    if (cursor.hasSelection() && cursor.atBlockStart()) {
        cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor);
    } else {
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    }
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

void Editor::doIncreaseIndent(QTextCursor cursor)
{
    QTextCursor end(document());
    end.setPosition(cursor.selectionEnd());
    cursor.setPosition(cursor.selectionStart());
    cursor.movePosition(QTextCursor::StartOfBlock);
    cursor.beginEditBlock();
    do {
        cursor.insertText(QString(" ").repeated(4));
        cursor.movePosition(QTextCursor::NextBlock);
    } while (cursor < end);
    cursor.endEditBlock();
}

void Editor::doDecreaseIndent(QTextCursor cursor)
{
    QTextCursor end(document());
    end.setPosition(cursor.selectionEnd());
    cursor.setPosition(cursor.selectionStart());
    cursor.movePosition(QTextCursor::StartOfBlock);
    cursor.beginEditBlock();
    do {
        int i = 4;
        while (i--) {
            if (!document()->characterAt(cursor.position()).isSpace())
                break;
            cursor.deleteChar();
        }
        cursor.movePosition(QTextCursor::NextBlock);
    } while (cursor < end);
    cursor.endEditBlock();
}

void Editor::doKillLine(QTextCursor cursor)
{
    doSelectBlocks(cursor);
    cursor.beginEditBlock();
    cursor.removeSelectedText();
    cursor.deleteChar();
    cursor.endEditBlock();
}

void Editor::doMoveText(QTextCursor selection, QTextCursor newPos)
{
    selection.beginEditBlock();
    QString text = selection.selectedText();
    selection.removeSelectedText();
    newPos.insertText(text);
    selection.endEditBlock();
}

bool Editor::findMore(const QString& exp, QTextDocument::FindFlags options)
{
    QTextCursor found = document()->find(exp, textCursor(), options);
    if (found.isNull()) {
        QTextCursor cursor(document());
        if (options & QTextDocument::FindBackward)
            cursor.movePosition(QTextCursor::End);
        found = document()->find(exp, cursor, options);
        // TODO: Visual wrap-around feedback
    }
    if (found.isNull()) {
        return false;
    } else {
        setTextCursor(found);
        return true;
    }
}

Editor::CursorPair Editor::findMatchingBackwards(QTextCursor cursor)
{
    cursor.clearSelection();
    cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor);
    QChar right = document()->characterAt(cursor.position());
    QChar left;
    if (right == ']') {
        left = '[';
    } else if (right == '}') {
        left = '{';
    } else if (right == ')') {
        left = '(';
    } else {
        return CursorPair();
    }
    int nested = 1;
    QTextCursor match = cursor;
    while (match.movePosition(QTextCursor::Left)) {
        QChar cur = document()->characterAt(match.position());
        if (cur == right) {
            ++nested;
        } else if (cur == left) {
            if (--nested == 0) {
                match.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);
                return qMakePair(cursor, match);
            }
        }
    }
    return CursorPair();
}

Editor::CursorPair Editor::findMatchingForward(QTextCursor cursor)
{
    cursor.clearSelection();
    cursor.movePosition(QTextCursor::Right);
    cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor);
    QChar left = document()->characterAt(cursor.position());
    QChar right;
    if (left == '[') {
        right = ']';
    } else if (left == '{') {
        right = '}';
    } else if (left == '(') {
        right = ')';
    } else {
        return CursorPair();
    }
    int nested = 1;
    QTextCursor match = cursor;
    while (match.movePosition(QTextCursor::Right)) {
        QChar cur = document()->characterAt(match.position());
        if (cur == left) {
            ++nested;
        } else if (cur == right) {
            if (--nested == 0) {
                match.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);
                return qMakePair(cursor, match);
            }
        }
    }
    return CursorPair();
}

void Editor::selectBlocks()
{
    QTextCursor cursor = textCursor();
    doSelectBlocks(cursor);
    setTextCursor(cursor);
}

void Editor::indentLine()
{
    doIndent(textCursor());
}

void Editor::increaseIndent()
{
    doIncreaseIndent(textCursor());
}

void Editor::decreaseIndent()
{
    doDecreaseIndent(textCursor());
}

void Editor::killLine()
{
    doKillLine(textCursor());
}

void Editor::newline()
{
    QTextCursor cursor = textCursor();
    cursor.beginEditBlock();
    cursor.insertBlock();
    doIndent(cursor);
    cursor.endEditBlock();
}

void Editor::smartNewline()
{
    QTextCursor cursor = textCursor();
    QRegExp exp("^(\\W*)((?=\\w)|$)");
    exp.indexIn(cursor.block().text());
    QString prefix = exp.cap().left(cursor.positionInBlock());

    cursor.beginEditBlock();
    cursor.insertBlock();
    cursor.insertText(prefix);
    cursor.endEditBlock();
}

void Editor::insertLineBefore()
{
    QTextCursor cursor = textCursor();
    cursor.beginEditBlock();
    cursor.movePosition(QTextCursor::StartOfBlock);
    cursor.insertBlock();
    cursor.movePosition(QTextCursor::PreviousBlock);
    doIndent(cursor);
    cursor.endEditBlock();
    setTextCursor(cursor);
}

void Editor::insertLineAfter()
{
    QTextCursor cursor = textCursor();
    cursor.beginEditBlock();
    cursor.movePosition(QTextCursor::EndOfBlock);
    cursor.insertBlock();
    doIndent(cursor);
    cursor.endEditBlock();
    setTextCursor(cursor);
}

void Editor::moveRegionUp()
{
    QTextCursor cursor = textCursor();
    doSelectBlocks(cursor);
    setTextCursor(cursor);
    QTextCursor lineBefore(document());
    lineBefore.setPosition(cursor.selectionStart());
    lineBefore.movePosition(QTextCursor::StartOfBlock);
    if (!lineBefore.movePosition(QTextCursor::PreviousBlock, QTextCursor::KeepAnchor))
        return; // Region already at beginning of file
    QTextCursor newPos(document());
    newPos.setPosition(cursor.selectionEnd());
    newPos.movePosition(QTextCursor::NextBlock);
    doMoveText(lineBefore, newPos);
}

void Editor::moveRegionDown()
{
    QTextCursor cursor = textCursor();
    doSelectBlocks(cursor);
    setTextCursor(cursor);
    QTextCursor lineAfter(document());
    lineAfter.setPosition(cursor.selectionEnd());
    lineAfter.movePosition(QTextCursor::NextBlock);
    if (!lineAfter.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor))
        return; // Region already at end of file
    QTextCursor newPos(document());
    newPos.setPosition(cursor.selectionStart());
    newPos.movePosition(QTextCursor::StartOfBlock);
    doMoveText(lineAfter, newPos);
}

void Editor::highlightMatching()
{
    QList<ExtraSelection> selections;
    QList<CursorPair> matching;

    matching << findMatchingBackwards(textCursor());
    matching << findMatchingForward(textCursor());

    foreach (CursorPair match, matching) {
        if (!match.second.isNull()) {
            ExtraSelection sel;
            sel.cursor = match.first;
            sel.format = match.first.charFormat();
            sel.format.setForeground(Qt::red); // FIXME theme colors?
            selections << sel;
            sel.cursor = match.second;
            sel.format = match.second.charFormat();
            sel.format.setBackground(Qt::green);
            sel.format.setForeground(Qt::red);
            selections << sel;
        }
    }
    setExtraSelections(selections);
}

void Editor::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Return) {
        if (e->modifiers() & Qt::ControlModifier) {
            if (e->modifiers() & Qt::ShiftModifier) {
                insertLineBefore();
            } else {
                insertLineAfter();
            }
        } else if (e->modifiers() & Qt::ShiftModifier) {
            smartNewline();
        } else {
            newline();
        }
        e->accept();
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
