#ifndef EDITOR_H
#define EDITOR_H

#include <QTextEdit>
#include <QMap>

class QTextDocument;

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
    QMap<QString, QTextDocument*> documents;
};

#endif // EDITOR_H
