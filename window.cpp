#include "window.h"
#include "navigator.h"

#include <QLayout>
#include <QTextBrowser>

Window::Window(QWidget *parent) :
    QWidget(parent)
{
    QVBoxLayout *vl = new QVBoxLayout(this);
    navigator = new Navigator(this);
    vl->addWidget(navigator);
    vl->addWidget(new QTextBrowser());
}
