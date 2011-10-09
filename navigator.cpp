#include "navigator.h"
#include "ui_navigator.h"

Navigator::Navigator(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Navigator)
{
    ui->setupUi(this);

    connect(ui->comboBox, SIGNAL(activated(QString)), this, SIGNAL(activated(QString)));
}

Navigator::~Navigator()
{
    delete ui;
}
