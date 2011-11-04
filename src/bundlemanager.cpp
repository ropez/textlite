#include "bundlemanager.h"
#include "highlighter.h"
#include "plistreader.h"

#include <QDir>

#include <QtDebug>

class BundleManagerPrivate
{
    friend class BundleManager;

    Theme theme;
    QString themeDirPath;

    QMap<QString, QString> fileTypes;
    QMap<QString, QVariantMap> syntaxData;
};

BundleManager::BundleManager(const QString& themeDirPath, QObject *parent) :
    QObject(parent),
    d(new BundleManagerPrivate)
{
    d->themeDirPath = themeDirPath;
}

BundleManager::~BundleManager()
{
}

Theme BundleManager::theme() const
{
    return d->theme;
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
                QString scopeName = syntaxData.value("scopeName").toString();
                d->syntaxData[scopeName] = syntaxData;
                QVariantList fileTypes = syntaxData.value("fileTypes").toList();
                foreach (QVariant type, fileTypes) {
                    d->fileTypes[type.toString()] = scopeName;
                }
            }
        }
    }
}

QVariantMap BundleManager::getSyntaxData(const QString& scopeName) const
{
    return d->syntaxData[scopeName];
}

Highlighter* BundleManager::getHighlighterForExtension(const QString& extension, QTextDocument* document)
{
    Highlighter* highlighter = new Highlighter(document, this);

    if (d->fileTypes.contains(extension)) {
        highlighter->readSyntaxData(d->fileTypes.value(extension));
    }

    return highlighter;
}

void BundleManager::readThemeFile(const QString& themeFile)
{
    d->theme.readThemeFile(d->themeDirPath + "/" + themeFile);
    emit themeChanged(d->theme);
}
