#include "navigator.h"

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

Navigator::Navigator(QWidget *parent)
    : QWidget(parent)
    , model(0)
{
    pathEdit = new QLineEdit(this);
    themeSelector = new QComboBox(this);

    QHBoxLayout* hl = new QHBoxLayout(this);
    hl->addWidget(new QLabel(tr("File"), this));
    hl->addWidget(pathEdit);
    hl->addWidget(themeSelector);
    hl->setContentsMargins(-1, 0, -1, 0);

    connect(pathEdit, SIGNAL(returnPressed()), this, SLOT(activate()));
    connect(themeSelector, SIGNAL(activated(QString)), this, SIGNAL(themeChange(QString)));

    // Create completer
    QCompleter* completer = new QCompleter(this);
    pathEdit->setCompleter(completer);

    refresh();
}

Navigator::~Navigator()
{
}

QString Navigator::fileName() const
{
    return pathEdit->text();
}

void Navigator::refresh()
{
    try {
        // Open Git repository
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
        delete model;
        model = new QGitIndexModel(repo.index(), this);

        // Enable completer
        pathEdit->completer()->setModel(model);
    } catch (const QGitException& e) {
        QMessageBox::critical(this, tr("Git operation failed"), e.message());
    }
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
