#include "navigator.h"
#include "ui_navigator.h"

#include <QCompleter>
#include <QFileSystemModel>
#include <QLineEdit>

#include <QtDebug>

Navigator::Navigator(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Navigator)
{
    ui->setupUi(this);

    connect(ui->comboBox, SIGNAL(returnPressed()), this, SLOT(activate()));

    QCompleter* completer = new QCompleter(this);
    QFileSystemModel* model = new QFileSystemModel(completer);
    model->setRootPath(QDir::currentPath());

    completer->setModel(model);
    ui->comboBox->setCompleter(completer);
}

Navigator::~Navigator()
{
    delete ui;
}

void Navigator::setFileFocus()
{
    ui->comboBox->setFocus();
    ui->comboBox->selectAll();
}

void Navigator::activate()
{
    emit activated(ui->comboBox->text());
}
