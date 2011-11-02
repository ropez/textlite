#ifndef HIGHLIGHTER_H
#define HIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QScopedPointer>
#include <QStack>

class BundleManager;
class HighlighterPrivate;

class Theme
{
public:
    void readThemeFile(const QString& themeFile);

    QTextCharFormat format(const QString& name) const;

    /**
      "string" => "string" -> ""
      "string.quoted" => "string.quited" -> "string" -> ""
      "string.koko","pun.ruby" => "string.koko pun.ruby", "string pun.ruby" -> "pun.ruby" -> "string pun" -> "pun" -> "string" -> ""
      "a", "b", "c" => "a b c" -> "b c" -> "a c" -> "c" -> "a b" -> "b" -> "a" -> ""
      */
    QTextCharFormat findFormat(const QStack<QString>& scope) const;

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
