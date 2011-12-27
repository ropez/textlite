#include "navigator.h"

#include <qgitrepository.h>
#include <qgitindexmodel.h>
#include <qgitexception.h>

#include <QCompleter>
#include <QComboBox>
#include <QStringListModel>
#include <QLayout>
#include <QLabel>
#include <QLineEdit>
#include <QDir>
#include <QMessageBox>

#include <QtDebug>

using namespace LibQGit2;

Navigator::Navigator(QWidget *parent) :
    QWidget(parent)
{
    pathEdit = new QLineEdit(this);
    themeSelector = new QComboBox(this);

    QHBoxLayout* hl = new QHBoxLayout(this);
    hl->addWidget(new QLabel(tr("File"), this));
    hl->addWidget(pathEdit);
    hl->addWidget(themeSelector);
    hl->setContentsMargins(-1, 0, -1, 0);

    connect(pathEdit, SIGNAL(returnPressed()), this, SLOT(activate()));

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
        QGitIndexModel* model = new QGitIndexModel(repo.index(), this);

        // Create completer
        QCompleter* completer = new QCompleter(this);
        completer->setModel(model);
        pathEdit->setCompleter(completer);
    } catch (const QGitException& e) {
        QMessageBox::critical(this, tr("Git operation failed"), e.message());
    }

    connect(themeSelector, SIGNAL(activated(QString)), this, SIGNAL(themeChange(QString)));
}

Navigator::~Navigator()
{
}

QString Navigator::fileName() const
{
    return pathEdit->text();
}

void Navigator::setFileFocus()
{
    pathEdit->setFocus();
    pathEdit->selectAll();
}

void Navigator::setFileName(const QString& fileName)
{
    pathEdit->setText(fileName);
}

void Navigator::setThemeNames(const QStringList& themeList)
{
    themeSelector->clear();
    themeSelector->addItems(themeList);

    // FIXME Temporary until theme is stored between sessions
    emit themeChange(themeSelector->currentText());
}

void Navigator::activate()
{
    emit activated(pathEdit->text());
}
