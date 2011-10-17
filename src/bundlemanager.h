#ifndef BUNDLEMANAGER_H
#define BUNDLEMANAGER_H

#include <QObject>

class Highlighter;
class QTextDocument;

class BundleManagerPrivate;

class BundleManager : public QObject
{
    Q_OBJECT
public:
    explicit BundleManager(QObject *parent = 0);
    ~BundleManager();

    void readBundles(const QString& path);

    Highlighter* getHighlighterForExtension(const QString& extension, QTextDocument* document);

signals:

public slots:

private:
    BundleManagerPrivate* d;
};

#endif // BUNDLEMANAGER_H
