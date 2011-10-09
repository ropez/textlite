#include "navigator.h"
#include "ui_navigator.h"

Navigator::Navigator(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Navigator)
{
    ui->setupUi(this);
}

Navigator::~Navigator()
{
    delete ui;
}
