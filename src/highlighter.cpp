#include "highlighter.h"
#include "bundlemanager.h"
#include "theme.h"
#include "scopeselector.h"
#include "grammar.h"
#include "ruledata.h"
#include "editor.h"

#include <QTextCharFormat>
#include <QList>
#include <QStack>
#include <QMap>

#include <QtDebug>

struct ContextItem {
    explicit ContextItem(RulePtr p = RulePtr()) : rule(p) {}

    RulePtr rule;
    QString formattedEndPattern;
    Regex end;
};

namespace {
typedef QString::const_iterator iter_t;
enum MatchType { Normal, Begin, End };

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
}

HighlighterContext::~HighlighterContext()
{
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
    d->root = d->grammar.compile(syntaxData, scopeName);
}

class SearchHelper
{
public:
    SearchHelper(iter_t base, iter_t end, iter_t index);

    const iter_t base;
    const iter_t end;
    const iter_t index;
    const int offset;

    Match foundMatch;
    MatchType foundMatchType;
    RulePtr foundRule;

    void searchPattern(RulePtr rule, const Regex& regex, MatchType type);
    void searchPatterns(RulePtr parentRule);
    void searchContext(const ContextItem& context);

private:
    Match match;
};

SearchHelper::SearchHelper(iter_t base, iter_t end, iter_t index)
    : base(base), end(end), index(index), offset(index - base)
{
}

void SearchHelper::searchPattern(RulePtr rule, const Regex& regex, MatchType type)
{
    if (regex.isValid()) {
        if (regex.search(base, end, index, end, match)) {
            if (foundMatch.isEmpty() || match.pos() < foundMatch.pos()) {
                foundRule = rule;
                foundMatchType = type;
                foundMatch.swap(match);
            }
        }
    }
}

void SearchHelper::searchPatterns(RulePtr parentRule)
{
    foreach (RulePtr rule, parentRule->patterns) {
        searchPattern(rule, rule->begin, Begin);
        searchPattern(rule, rule->match, Normal);

        if (rule->include) {
            searchPatterns(rule->include);
        }

        if (!foundMatch.isEmpty()) {
            Q_ASSERT(foundMatch.pos() >= offset);
            if (foundMatch.pos() == offset)
                break; // Don't need to continue
        }
    }
}

void SearchHelper::searchContext(const ContextItem& context)
{
    searchPattern(context.rule, context.end, End);
    searchPatterns(context.rule);
}

void Highlighter::highlightBlock(const QString &text)
{
    if (!d->root)
        return;

    QStack<ContextItem> contextStack;
    QStack<QString> scope;
    EditorBlockData *prevBlockData = EditorBlockData::forBlock(currentBlock().previous());
    if (prevBlockData && prevBlockData->context) {
        HighlighterContext* ctx = prevBlockData->context.data();
        contextStack = ctx->stack;
        scope = ctx->scope;
    } else {
        contextStack.push(ContextItem(d->root));
    }

    const iter_t base = text.begin();
    const iter_t end = text.end();

    iter_t index = base;
    while (true) {
        Q_ASSERT(contextStack.size() > 0);

        // Find next pattern
        SearchHelper s(base, end, index);
        s.searchContext(contextStack.top());

        // Did we find anything to highlight?
        if (s.foundMatch.isEmpty()) {
            setFormat(s.offset, text.length() - s.offset, d->theme.findFormat(scope));
            break;
        }

        // Highlight skipped section
        if (s.foundMatch.pos() != s.offset) {
            setFormat(s.offset, s.foundMatch.pos() - s.offset, d->theme.findFormat(scope));
        }

        // Leave nested context
        if (s.foundMatchType == End) {
            contextStack.pop();
            scope.pop();
        }

        Q_ASSERT(base + s.foundMatch.pos() <= end);

        scope.push(s.foundRule->name);
        setFormat(s.foundMatch.pos(), s.foundMatch.len(), d->theme.findFormat(scope));

        QMap<int, RulePtr> captures;
        switch (s.foundMatchType) {
        case Normal:
            captures = s.foundRule->captures;
            break;
        case Begin:
            captures = s.foundRule->beginCaptures;
            break;
        case End:
            captures = s.foundRule->endCaptures;
            break;
        }

        // Highlight captures
        for (int c = 1; c < s.foundMatch.size(); c++) {
            if (s.foundMatch.matched(c)) {
                if (captures.contains(c)) {
                    Q_ASSERT(s.foundMatch.pos(c) >= s.foundMatch.pos());
                    Q_ASSERT(s.foundMatch.pos(c) + s.foundMatch.len(c) <= s.foundMatch.pos() + s.foundMatch.len());

                    scope.push(captures[c]->name);
                    setFormat(s.foundMatch.pos(c), s.foundMatch.len(c), d->theme.findFormat(scope));
                    scope.pop();
                }
            }
        }
        scope.pop();

        // Enter nested context
        if (s.foundMatchType == Begin) {
            // Compile the regular expression that will end this context,
            // and may include captures from the found match
            ContextItem item(s.foundRule);
            item.formattedEndPattern = s.foundMatch.format(s.foundRule->endPattern);
            item.end.setPattern(item.formattedEndPattern);
            contextStack.push(item);
            scope.push(s.foundRule->contentName);
        }

        index = base + s.foundMatch.pos() + s.foundMatch.len();
    }

    EditorBlockData *currentBlockData = EditorBlockData::forBlock(currentBlock());
    Q_ASSERT(currentBlockData != 0);
    if (contextStack.size() > 1) {
        currentBlockData->context.reset(new HighlighterContext);
        currentBlockData->context->stack = contextStack;
        currentBlockData->context->scope = scope;
        setCurrentBlockState(_Hash(contextStack));
    } else {
        currentBlockData->context.reset();
        setCurrentBlockState(-1);
    }
}
