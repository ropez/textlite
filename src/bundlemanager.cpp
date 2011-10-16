#include "bundlemanager.h"
#include "highlighter.h"

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
