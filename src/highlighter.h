#ifndef HIGHLIGHTER_H
#define HIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QScopedPointer>

class BundleManager;
class HighlighterPrivate;

class Theme
{
public:
    void readThemeFile(const QString& themeFile);

    QTextCharFormat format(const QString& name) const;
    QTextCharFormat mergeFormat(const QString& name, const QTextCharFormat& baseFormat) const;

private:
    QHash<QString, QTextCharFormat> data;
};

class Highlighter : public QSyntaxHighlighter
{
    Q_OBJECT
public:
    explicit Highlighter(QTextDocument *document, BundleManager* bundleManager);
    ~Highlighter();

public slots:
    void setTheme(const Theme& theme);
    void readSyntaxFile(const QString& syntaxFile);

protected:
    void highlightBlock(const QString &text);

private:
    QScopedPointer<HighlighterPrivate> d;
};

#endif // HIGHLIGHTER_H
