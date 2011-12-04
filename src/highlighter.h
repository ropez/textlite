#ifndef HIGHLIGHTER_H
#define HIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QScopedPointer>

class Theme;
class BundleManager;
class HighlighterPrivate;


class Highlighter : public QSyntaxHighlighter
{
    Q_OBJECT
public:
    explicit Highlighter(QTextDocument *document, BundleManager* bundleManager);
    ~Highlighter();

public slots:
    void setTheme(const Theme& theme);
    void readSyntaxData(const QString& scopeName);

protected:
    void highlightBlock(const QString &text);

private:
    QScopedPointer<HighlighterPrivate> d;
};

#endif // HIGHLIGHTER_H
