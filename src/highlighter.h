#ifndef HIGHLIGHTER_H
#define HIGHLIGHTER_H

#include <QSyntaxHighlighter>

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
    explicit Highlighter(QTextDocument *document);
    ~Highlighter();

    void readThemeFile(const QString& themeFile);
    void readSyntaxFile(const QString& syntaxFile);

protected:
    void highlightBlock(const QString &text);

private:
    HighlighterPrivate* d;
};

#endif // HIGHLIGHTER_H
