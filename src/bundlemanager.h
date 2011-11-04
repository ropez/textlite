#ifndef BUNDLEMANAGER_H
#define BUNDLEMANAGER_H

#include <QObject>
#include <QScopedPointer>
#include <QVariant>

class Theme;
class Highlighter;
class QTextDocument;

class BundleManagerPrivate;

class BundleManager : public QObject
{
    Q_OBJECT
public:
    explicit BundleManager(const QString& themeDirPath, QObject *parent = 0);
    ~BundleManager();

    Theme theme() const;

    void readBundles(const QString& path);

    QVariantMap getSyntaxData(const QString& scopeName) const;

    Highlighter* getHighlighterForExtension(const QString& extension, QTextDocument* document);

signals:
    void themeChanged(const Theme& theme);

public slots:
    void readThemeFile(const QString& themeFile);

private:
    QScopedPointer<BundleManagerPrivate> d;
};

#endif // BUNDLEMANAGER_H
