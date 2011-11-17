#include "navigator.h"
#include "ui_navigator.h"

#include <qgitrepository.h>
#include <qgitindex.h>
#include <qgitindexentry.h>

#include <QCompleter>
#include <QFileSystemModel>
#include <QStringListModel>
#include <QLineEdit>
#include <QDir>

#include <QtDebug>

using namespace LibQGit2;

Navigator::Navigator(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Navigator)
{
    ui->setupUi(this);

    connect(ui->comboBox, SIGNAL(returnPressed()), this, SLOT(activate()));

    QCompleter* completer = new QCompleter(this);
//    QFileSystemModel* model = new QFileSystemModel(completer);
//    model->setRootPath(QDir::currentPath());

    QStringList gitEntryList;
    QGitRepository r;
    if (r.open(".git") != 0) {
        qWarning() << "Git open failed";
    }

    QGitIndex index(r);
    index.read();

    for (int i = 0; i < index.entryCount(); i++) {
        QGitIndexEntry e = index.get(i);
        gitEntryList.append(QString(e.path()));
        qDebug() << e.id().format() << e.path();
    }
    QStringListModel* model = new QStringListModel(gitEntryList, this);


    completer->setModel(model);
    ui->comboBox->setCompleter(completer);

    QDir themeDir("redcar-bundles/Themes");
    themeDir.setFilter(QDir::Files);
    themeDir.setNameFilters(QStringList() << "*.tmTheme");
    QStringList themeList = themeDir.entryList();
    ui->themeSelector->addItems(themeList);

    connect(ui->themeSelector, SIGNAL(activated(QString)), this, SIGNAL(themeChange(QString)));
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
