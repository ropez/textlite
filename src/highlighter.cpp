#include "highlighter.h"
#include "bundlemanager.h"
#include "theme.h"
#include "scopeselector.h"
#include "grammar.h"

#include <QTextCharFormat>
#include <QList>
#include <QStack>
#include <QMap>

#include <QtDebug>

namespace {
typedef QString::const_iterator iter_t;
enum MatchType { Normal, Begin, End };

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
    d->grammar.compile(syntaxData, scopeName);
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
