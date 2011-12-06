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

    QMap<QTextCursor, QString> scopes;

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
    void doKillLine(QTextCursor cursor);


public slots:
    void indentLine();
    void killLine();

protected:
    void keyPressEvent(QKeyEvent* e);
    bool event(QEvent *e);

private:
};

#endif // EDITOR_H
