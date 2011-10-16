#ifndef WINDOW_H
#define WINDOW_H

#include <QWidget>

class Navigator;
class Editor;


class Window : public QWidget
{
    Q_OBJECT
public:
    explicit Window(QWidget *parent = 0);

private:
    Navigator* navigator;
    Editor* editor;
};

#endif // WINDOW_H
