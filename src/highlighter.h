#ifndef HIGHLIGHTER_H
#define HIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QScopedPointer>
#include <QStack>

class Theme;
class BundleManager;
class GrammarPrivate;
class HighlighterPrivate;

struct RuleData;
typedef QSharedPointer<RuleData> RulePtr;
typedef QWeakPointer<RuleData> WeakRulePtr;


class Grammar
{
public:
    Grammar();
    ~Grammar();

    RulePtr root() const;

    void readSyntaxData(const QVariantMap& syntaxData);

    void resolveChildRules(const QMap<QString, QVariantMap>& syntaxData);
    void resolveChildRules(const QMap<QString, QVariantMap>& syntaxData, RulePtr baseRule);
    void resolveChildRules(const QMap<QString, QVariantMap>& syntaxData, RulePtr baseRule, RulePtr parentRule);

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
