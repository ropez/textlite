#include "bundlemanager.h"
#include "highlighter.h"

#include <QDir>

#include <QtDebug>

Highlighter* Bundle::createHighlighter(QTextDocument *document) const
{
    Highlighter* highlighter = new Highlighter(document);

    // XXX
    highlighter->readThemeFile("redcar-bundles/Themes/Espresso.tmTheme");
    highlighter->readSyntaxFile("redcar-bundles/Bundles/XML.tmbundle/Syntaxes/XML.plist");

    return highlighter;
}

BundleManager::BundleManager(QObject *parent) :
    QObject(parent)
{
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
                qDebug() << syntaxDir.filePath(file);
            }
        }
    }

    //    QFile fileList("files.txt");
    //    fileList.open(QFile::ReadOnly);
    //    while (!fileList.atEnd()) {
    //        QByteArray line = fileList.readLine().trimmed();
    //        qDebug() << line;
    //        PlistReader reader;
    //        QVariant map = reader.read(line);
    //        Q_ASSERT(map.isValid());
    //    }
}

Bundle* BundleManager::getBundleForExtension(const QString& extension)
{
    return new Bundle();
}
