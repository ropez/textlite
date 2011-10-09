#ifndef EDITOR_H
#define EDITOR_H

#include <QTextEdit>

class Editor : public QTextEdit
{
    Q_OBJECT
public:
    explicit Editor(QWidget *parent = 0);

public slots:
    void setFileName(const QString& name);
    void saveFile();

private:
    QString filename;
};

#endif // EDITOR_H
