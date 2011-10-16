#ifndef BUNDLEMANAGER_H
#define BUNDLEMANAGER_H

#include <QObject>

class Highlighter;
class QTextDocument;

class Bundle
{
public:
    Highlighter* createHighlighter(QTextDocument* document) const;
};

class BundleManager : public QObject
{
    Q_OBJECT
public:
    explicit BundleManager(QObject *parent = 0);

    void readBundles(const QByteArray& path);

    Bundle* getBundleForExtension(const QByteArray& extension);

signals:

public slots:

};

#endif // BUNDLEMANAGER_H
