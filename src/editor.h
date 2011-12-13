#ifndef EDITOR_H
#define EDITOR_H

#include <QTextEdit>
#include <QtGui/QTextBlockUserData>

class HighlighterContext;
class EditorBlockData : public QTextBlockUserData
{
public:
    EditorBlockData();
    ~EditorBlockData();

    static EditorBlockData* forBlock(QTextBlock block);

    QMap<QTextCursor, QStringList> scopes;

    QScopedPointer<HighlighterContext> context;
};

class Editor : public QTextEdit
{
    Q_OBJECT
public:
    explicit Editor(QWidget *parent = 0);

    QString scopeForCursor(const QTextCursor& cursor) const;

    bool currentIndent(const QTextCursor& cursor, int* indent) const;
    bool isLeadingWhitespace(const QTextCursor& cursor) const;

    void doIndent(QTextCursor c);
    void doIncreaseIndent(QTextCursor cursor);
    void doDecreaseIndent(QTextCursor cursor);
    void doKillLine(QTextCursor cursor);

    /**
     * This function behaves similar to QTextEdit::find() except that it has the ability to
     * continue from the beginning when the end of the document is reached without finding the
     * text.
     */
    bool findMore(const QString& exp, QTextDocument::FindFlags options = 0);

public slots:
    void indentLine();
    void increaseIndent();
    void decreaseIndent();
    void killLine();

    void smartNewline();

protected:
    void keyPressEvent(QKeyEvent* e);
    bool event(QEvent *e);

private:
};

#endif // EDITOR_H
