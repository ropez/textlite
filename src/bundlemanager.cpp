#include "bundlemanager.h"
#include "highlighter.h"
#include "plistreader.h"

#include <QDir>

#include <QtDebug>

class BundleManagerPrivate
{
    friend class BundleManager;

    QMap<QString, QString> bundles;
};

BundleManager::BundleManager(QObject *parent) :
    QObject(parent),
    d(new BundleManagerPrivate)
{
}

BundleManager::~BundleManager()
{
    delete d;
}

void BundleManager::readBundles(const QString &path)
{
    QDir bundleDir(path);
    bundleDir.setFilter(QDir::Dirs);
    bundleDir.setNameFilters(QStringList() << "*.tmbundle");
    foreach (QString bundleName, bundleDir.entryList()) {
        QString bundlePath = bundleDir.filePath(bundleName);
        QDir syntaxDir(bundlePath + "/Syntaxes");
        if (syntaxDir.exists()) {
            syntaxDir.setFilter(QDir::Files);
            QStringList nameFilters;
            nameFilters << "*.plist";
            nameFilters << "*.tmLanguage";
            syntaxDir.setNameFilters(nameFilters);
            foreach (QString file, syntaxDir.entryList()) {
                QString syntaxFile = syntaxDir.filePath(file);
                PlistReader reader;
                QVariantMap syntaxData = reader.read(syntaxFile).toMap();
                QVariantList fileTypes = syntaxData.value("fileTypes").toList();
                foreach (QVariant type, fileTypes) {
                    d->bundles[type.toString()] = syntaxFile;
                }
            }
        }
    }
}

Highlighter* BundleManager::getHighlighterForExtension(const QString& extension, QTextDocument* document)
{
    Highlighter* highlighter = new Highlighter(document);

    // XXX
    highlighter->readThemeFile("Espresso.tmTheme");

    if (d->bundles.contains(extension)) {
        highlighter->readSyntaxFile(d->bundles.value(extension));
    }

    return highlighter;
}
