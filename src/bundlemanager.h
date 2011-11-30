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
    explicit BundleManager(QObject *parent = 0);
    ~BundleManager();

    Theme theme() const;
    QStringList themeNames() const;

    void readThemes(const QString& path);
    void readBundles(const QString& path);

    QMap<QString, QVariantMap> getSyntaxData() const;

    Highlighter* getHighlighterForExtension(const QString& extension, QTextDocument* document);

signals:
    void themeChanged(const Theme& theme);

public slots:
    void setThemeName(const QString& themeName);

private:
    QScopedPointer<BundleManagerPrivate> d;
};

#endif // BUNDLEMANAGER_H
