#ifndef EDITOR_H
#define EDITOR_H

#include <QTextEdit>

class Editor : public QTextEdit
{
    Q_OBJECT
public:
    explicit Editor(QWidget *parent = 0);

    bool currentIndent(const QTextCursor& cursor, int* indent) const;
    void doIndent(QTextCursor c);

public slots:
    void indentLine();

protected:
    void keyPressEvent(QKeyEvent* e);

private:
};

#endif // EDITOR_H
