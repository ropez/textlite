#include "navigator.h"
#include "ui_navigator.h"

#include <QLineEdit>

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

void Navigator::setFileFocus()
{
    ui->comboBox->setFocus();
    ui->comboBox->lineEdit()->selectAll();
}
