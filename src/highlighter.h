#ifndef HIGHLIGHTER_H
#define HIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QScopedPointer>
#include <QStack>

class BundleManager;
class GrammarPrivate;
class HighlighterPrivate;

class ScopeSelector;
class ThemePrivate;

class Theme
{
public:
    Theme();
    ~Theme();

    void setThemeData(const QVariantMap& themeData);

    QTextCharFormat format(const QString& name) const;

    /**
      "string" => "string" -> ""
      "string.quoted" => "string.quited" -> "string" -> ""
      "string.koko","pun.ruby" => "string.koko pun.ruby", "string pun.ruby" -> "pun.ruby" -> "string pun" -> "pun" -> "string" -> ""
      "a", "b", "c" => "a b c" -> "b c" -> "a c" -> "c" -> "a b" -> "b" -> "a" -> ""
      */
    QTextCharFormat findFormat(const ScopeSelector& scope) const;

private:
    QSharedPointer<ThemePrivate> d;
};

class Grammar
{
public:
    Grammar();
    ~Grammar();

    void readSyntaxData(const QVariantMap& syntaxData);

private:
    friend class Highlighter;
    friend class HighlighterPrivate;

    QSharedPointer<GrammarPrivate> d;
};

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
