#include "navigator.h"
#include "ui_navigator.h"

#include <qgitrepository.h>
#include <qgitindex.h>
#include <qgitindexentry.h>
#include <qgitexception.h>

#include <QCompleter>
#include <QStringListModel>
#include <QLineEdit>
#include <QDir>
#include <QMessageBox>

#include <QtDebug>

using namespace LibQGit2;

Navigator::Navigator(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Navigator)
{
    ui->setupUi(this);

    connect(ui->comboBox, SIGNAL(returnPressed()), this, SLOT(activate()));

    try {
        // Open Git repository
        QGitRepository repo;
        repo.discoverAndOpen(QDir::currentPath());

        // Debugging
        qDebug() << repo.isHeadDetached();
        qDebug() << repo.isHeadOrphan();
        qDebug() << repo.isEmpty();
        qDebug() << repo.isBare();
        qDebug() << repo.path();
        qDebug() << repo.indexPath();
        qDebug() << repo.databasePath();
        qDebug() << repo.workDirPath();

        // Change working directory, so that files can be found
        QDir::setCurrent(repo.workDirPath());

        // Read Git index
        QGitIndex index = repo.index();
        index.read();

        // Create list of index entries for filename autocompletion
        QStringList gitEntryList;
        for (size_t i = 0; i < index.entryCount(); i++) {
            QGitIndexEntry e = index.get(i);
            gitEntryList.append(QString(e.path()));
            qDebug() << e.id().format() << "=>" << e.path();
        }
        QStringListModel* model = new QStringListModel(gitEntryList, this);

        // Create completer
        QCompleter* completer = new QCompleter(this);
        completer->setModel(model);
        ui->comboBox->setCompleter(completer);
    } catch (const QGitException& e) {
        QMessageBox::critical(this, tr("Git operation failed"), e.message());
    }

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

QString Navigator::fileName() const
{
    return ui->comboBox->text();
}

void Navigator::setFileFocus()
{
    ui->comboBox->setFocus();
    ui->comboBox->selectAll();
}

void Navigator::setFileName(const QString& fileName)
{
    ui->comboBox->setText(fileName);
}

void Navigator::activate()
{
    emit activated(ui->comboBox->text());
}
