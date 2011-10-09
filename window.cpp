#include "window.h"
#include "navigator.h"
#include "editor.h"

#include <QLayout>

Window::Window(QWidget *parent) :
    QWidget(parent)
{
    QVBoxLayout *vl = new QVBoxLayout(this);
    navigator = new Navigator(this);
    editor = new Editor(this);
    vl->addWidget(navigator);
    vl->addWidget(editor);

    connect(navigator, SIGNAL(activated(QString)), editor, SLOT(setFileName(QString)));
}
