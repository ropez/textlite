#include "highlighter.h"
#include "bundlemanager.h"
#include "theme.h"
#include "scopeselector.h"

#include <QTextCharFormat>
#include <QList>
#include <QStack>
#include <QMap>

#include <QtDebug>

#include "regex.h"

namespace {
typedef QString::const_iterator iter_t;
enum MatchType { Normal, Begin, End };
}

struct RuleData {
    bool visited;
    RuleData() : visited(false) {}

    QString name;
    QString contentName;
    QString includeName;
    QString beginPattern;
    QString endPattern;
    QString matchPattern;
    Regex begin;
    Regex match;
    QMap<int, RulePtr> captures;
    QMap<int, RulePtr> beginCaptures;
    QMap<int, RulePtr> endCaptures;
    QList<RulePtr> patterns;
    WeakRulePtr include;
};

namespace {

struct ContextItem {
    explicit ContextItem(RulePtr p = RulePtr()) : rule(p) {}

    RulePtr rule;
    QString formattedEndPattern;
    Regex end;
};

void _HashCombine(int& h, int hh) {
    h = (h << 4) ^ (h >> 28) ^ hh;
}

int _Hash(const QString& str) {
    int h = 0;
    for (QString::const_iterator it = str.begin(); it != str.end(); ++it) {
        _HashCombine(h, static_cast<int>(it->unicode()));
    }
    return h;
}

int _Hash(const Regex& regex) {
    return _Hash(regex.pattern());
}

int _Hash(const ContextItem& item) {
    int h = 0;
    _HashCombine(h, _Hash(item.rule->begin));
    _HashCombine(h, _Hash(item.end));
    return h;
}

int _Hash(const QStack<ContextItem>& stack) {
    int h = 0;
    foreach (const ContextItem& item, stack) {
        _HashCombine(h, _Hash(item));
    }
    return h;
}

struct Context : public QTextBlockUserData {
    QStack<ContextItem> stack;
    QStack<QString> scope;
};

}

class GrammarPrivate
{
    friend class Grammar;

    RulePtr root;
    QMap<QString, RulePtr> repository;

    QMap<QString, Grammar> grammars;

    QMap<int, RulePtr> makeCaptures(const QVariantMap& capturesData);
    RulePtr makeRule(const QVariantMap& ruleData);
    QList<RulePtr> makeRuleList(const QVariantList& ruleListData);
};

Grammar::Grammar() :
    d(new GrammarPrivate)
{
}

Grammar::~Grammar()
{
}

RulePtr Grammar::root() const
{
    return d->root;
}

void Grammar::readSyntaxData(const QVariantMap& syntaxData)
{
    QVariantMap rootData;
    rootData["patterns"] = syntaxData.value("patterns");
    d->root = d->makeRule(rootData);

    QVariantMap repositoryData = syntaxData.value("repository").toMap();
    QMapIterator<QString, QVariant> iter(repositoryData);
    while (iter.hasNext()) {
        iter.next();
        QVariantMap ruleData = iter.value().toMap();
        d->repository["#" + iter.key()] = d->makeRule(ruleData);
    }
}

class HighlighterPrivate
{
    friend class Highlighter;

    BundleManager* bundleManager;

    RulePtr root;
    Grammar grammar;

    Theme theme;

    void searchPatterns(const RulePtr& parentRule, const iter_t index, const iter_t end, const iter_t base,
                        RulePtr& foundRule, MatchType& foundMatchType, Match& foundMatch);
};

QMap<int, RulePtr> GrammarPrivate::makeCaptures(const QVariantMap& capturesData)
{
    QMap<int, RulePtr> captures;
    QMapIterator<QString, QVariant> iter(capturesData);
    while (iter.hasNext()) {
        iter.next();
        int num = iter.key().toInt();
        QVariantMap data = iter.value().toMap();
        captures[num] = makeRule(data);
    }
    return captures;
}

RulePtr GrammarPrivate::makeRule(const QVariantMap& ruleData)
{
    RulePtr rule(new RuleData);

    rule->name = ruleData.value("name").toString();
    if (ruleData.contains("contentName"))
        rule->contentName = ruleData.value("contentName").toString();
    else
        rule->contentName = rule->name;
    rule->includeName = ruleData.value("include").toString();
    if (ruleData.contains("begin")) {
        rule->beginPattern = ruleData.value("begin").toString();
        rule->begin.setPattern(rule->beginPattern);
    }
    if (ruleData.contains("end")) {
        rule->endPattern = ruleData.value("end").toString();
    }
    if (ruleData.contains("match")) {
        rule->matchPattern = ruleData.value("match").toString();
        rule->match.setPattern(rule->matchPattern);
    }
    QVariant ruleListData = ruleData.value("patterns");
    if (ruleListData.isValid()) {
        rule->patterns = makeRuleList(ruleListData.toList());
    }

    QVariant capturesData = ruleData.value("captures");
    rule->captures = makeCaptures(capturesData.toMap());
    QVariant beginCapturesData = ruleData.value("beginCaptures");
    if (beginCapturesData.isValid())
        rule->beginCaptures = makeCaptures(beginCapturesData.toMap());
    else
        rule->beginCaptures = rule->captures;
    QVariant endCapturesData = ruleData.value("endCaptures");
    if (endCapturesData.isValid())
        rule->endCaptures = makeCaptures(endCapturesData.toMap());
    else
        rule->endCaptures = rule->captures;

    return rule;
}

QList<RulePtr> GrammarPrivate::makeRuleList(const QVariantList& ruleListData)
{
    QList<RulePtr> rules;
    QListIterator<QVariant> iter(ruleListData);
    while (iter.hasNext()) {
        QVariantMap ruleData = iter.next().toMap();
        rules << makeRule(ruleData);
    }
    return rules;
}

void Grammar::resolveChildRules(const QMap<QString, QVariantMap>& syntaxData)
{
    resolveChildRules(syntaxData, d->root, d->root);
}


void Grammar::resolveChildRules(const QMap<QString, QVariantMap>& syntaxData,
                                RulePtr baseRule)
{
    resolveChildRules(syntaxData, baseRule, d->root);
}

void Grammar::resolveChildRules(const QMap<QString, QVariantMap>& syntaxData,
                                RulePtr baseRule, RulePtr parentRule)
{
    if (parentRule->visited) return;
    parentRule->visited = true;

    QListIterator<RulePtr> iter(parentRule->patterns);
    while (iter.hasNext()) {
        RulePtr rule = iter.next();
        if (rule->includeName != QString()) {
            if (rule->includeName == "$base") {
                rule->include = baseRule;
            } else if (rule->includeName == "$self") {
                rule->include = d->root;
            } else if (d->repository.contains(rule->includeName)) {
                rule->include = d->repository.value(rule->includeName);
                resolveChildRules(syntaxData, baseRule, rule->include);
            } else if (d->grammars.contains(rule->includeName)) {
                rule->include = d->grammars.value(rule->includeName).root();
            } else if (syntaxData.contains(rule->includeName)) {
                QVariantMap data = syntaxData[rule->includeName];
                Grammar g;
                g.readSyntaxData(data);
                g.resolveChildRules(syntaxData, baseRule);
                rule->include = g.root();
                d->grammars[rule->includeName] = g;
            } else {
                qWarning() << "Pattern not in repository" << rule->includeName;
            }
        } else {
            resolveChildRules(syntaxData, baseRule, rule);
        }
    }
}

Highlighter::Highlighter(QTextDocument* document, BundleManager *bundleManager) :
    QSyntaxHighlighter(document),
    d(new HighlighterPrivate)
{
    d->bundleManager = bundleManager;
    d->theme = d->bundleManager->theme();
    connect(d->bundleManager, SIGNAL(themeChanged(Theme)), this, SLOT(setTheme(Theme)));
}

Highlighter::~Highlighter()
{
}

void Highlighter::setTheme(const Theme& theme)
{
    d->theme = theme;
    rehighlight();
}

void Highlighter::readSyntaxData(const QString& scopeName)
{
    QMap<QString, QVariantMap> syntaxData = d->bundleManager->getSyntaxData();
    d->grammar.readSyntaxData(syntaxData[scopeName]);
    d->grammar.resolveChildRules(syntaxData);
}

void HighlighterPrivate::searchPatterns(const RulePtr& parentRule, const iter_t index, const iter_t end, const iter_t base,
                                        RulePtr& foundRule, MatchType& foundMatchType, Match& foundMatch)
{
    const int offset = index - base;

    Match match;
    foreach (RulePtr rule, parentRule->patterns) {
        if (rule->begin.isValid()) {
            if (rule->begin.search(base, end, index, end, match)) {
                if (foundMatch.isEmpty() || match.pos() < foundMatch.pos()) {
                    foundRule = rule;
                    foundMatchType = Begin;
                    foundMatch.swap(match);
                }
            }
        } else if (rule->match.isValid()) {
            if (rule->match.search(base, end, index, end, match)) {
                if (foundMatch.isEmpty() || match.pos() < foundMatch.pos()) {
                    foundRule = rule;
                    foundMatchType = Normal;
                    foundMatch.swap(match);
                }
            }
        } else if (rule->include) {
            searchPatterns(rule->include, index, end, base, foundRule, foundMatchType, foundMatch);
        }

        if (!foundMatch.isEmpty()) {
            Q_ASSERT(foundMatch.pos() >= offset);
            if (foundMatch.pos() == offset)
                break; // Don't need to continue
        }
    }
}

void Highlighter::highlightBlock(const QString &text)
{
    if (!d->grammar.root())
        return;

    QStack<ContextItem> contextStack;
    QStack<QString> scope;
    QTextBlock prevBlock = currentBlock().previous();
    if (prevBlock.userData()) {
        Context* ctx = static_cast<Context*>(prevBlock.userData());
        contextStack = ctx->stack;
        scope = ctx->scope;
    } else {
        contextStack.push(ContextItem(d->grammar.root()));
    }


    const iter_t base = text.begin();
    const iter_t end = text.end();

    iter_t index = base;
    while (true) {
        Q_ASSERT(contextStack.size() > 0);

        const int offset = index - base;

        RulePtr foundRule;
        MatchType foundMatchType = Normal;
        Match foundMatch;

        {
            ContextItem& context = contextStack.top();

            if (context.end.isValid()) {
                RulePtr rule = context.rule;
                Match match;
                if (context.end.search(base, end, index, end, match)) {
                    foundRule = rule;
                    foundMatchType = End;
                    foundMatch.swap(match);
                }
            }

            d->searchPatterns(context.rule, index, end, base, foundRule, foundMatchType, foundMatch);
        }

        // Did we find anything to highlight?
        if (foundMatch.isEmpty()) {
            setFormat(offset, text.length() - offset, d->theme.findFormat(scope));
            break;
        }

        // Highlight skipped section
        if (foundMatch.pos() != offset) {
            setFormat(offset, foundMatch.pos() - offset, d->theme.findFormat(scope));
        }

        // Leave nested context
        if (foundMatchType == End) {
            contextStack.pop();
            scope.pop();
        }

        Q_ASSERT(base + foundMatch.pos() <= end);

        scope.push(foundRule->name);
        setFormat(foundMatch.pos(), foundMatch.len(), d->theme.findFormat(scope));

        QMap<int, RulePtr> captures;
        switch (foundMatchType) {
        case Normal:
            captures = foundRule->captures;
            break;
        case Begin:
            captures = foundRule->beginCaptures;
            break;
        case End:
            captures = foundRule->endCaptures;
            break;
        }

        // Highlight captures
        for (int c = 1; c < foundMatch.size(); c++) {
            if (foundMatch.matched(c)) {
                if (captures.contains(c)) {
                    Q_ASSERT(foundMatch.pos(c) >= foundMatch.pos());
                    Q_ASSERT(foundMatch.pos(c) + foundMatch.len(c) <= foundMatch.pos() + foundMatch.len());

                    scope.push(captures[c]->name);
                    setFormat(foundMatch.pos(c), foundMatch.len(c), d->theme.findFormat(scope));
                    scope.pop();
                }
            }
        }
        scope.pop();

        // Enter nested context
        if (foundMatchType == Begin) {
            // Compile the regular expression that will end this context,
            // and may include captures from the found match
            ContextItem item(foundRule);
            item.formattedEndPattern = foundMatch.format(foundRule->endPattern);
            item.end.setPattern(item.formattedEndPattern);
            contextStack.push(item);
            scope.push(foundRule->contentName);
        }

        index = base + foundMatch.pos() + foundMatch.len();
    }

    if (contextStack.size() > 1) {
        Context* ctx = new Context;
        ctx->stack = contextStack;
        ctx->scope = scope;
        setCurrentBlockUserData(ctx);
        setCurrentBlockState(_Hash(ctx->stack));
    } else {
        setCurrentBlockUserData(0);
        setCurrentBlockState(-1);
    }
}
