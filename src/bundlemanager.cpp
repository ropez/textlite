#include "bundlemanager.h"
#include "highlighter.h"
#include "plistreader.h"

#include <QDir>

#include <QtDebug>

class BundleManagerPrivate
{
    friend class BundleManager;

    Theme theme;

    QMap<QString, QString> fileTypes;
    QMap<QString, QVariantMap> themeData;
    QMap<QString, QVariantMap> syntaxData;
};

BundleManager::BundleManager(QObject *parent) :
    QObject(parent),
    d(new BundleManagerPrivate)
{
}

BundleManager::~BundleManager()
{
}

Theme BundleManager::theme() const
{
    return d->theme;
}

QStringList BundleManager::themeNames() const
{
    return d->themeData.keys();
}

void BundleManager::readThemes(const QString& path)
{
    QDir themeDir(path);
    themeDir.setFilter(QDir::Files);
    themeDir.setNameFilters(QStringList() << "*.tmTheme");
    foreach (QString themeFileName, themeDir.entryList()) {
        QString themeFilePath = path + "/" + themeFileName;
        PlistReader reader;
        QVariantMap themeData = reader.read(themeFilePath).toMap();
        QString themeName = themeData["name"].toString();
        if (!themeName.isEmpty()) {
            d->themeData[themeName] = themeData;
        }
    }
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

void BundleManager::setThemeName(const QString& themeName)
{
    d->theme.setThemeData(d->themeData[themeName]);
    emit themeChanged(d->theme);
}
