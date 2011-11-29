#include "highlighter.h"
#include "bundlemanager.h"

#include <QTextCharFormat>
#include <QList>
#include <QStack>
#include <QMap>

#include <QtDebug>

#include "regex.h"

namespace {
typedef QString::const_iterator iter_t;
enum MatchType { Normal, Begin, End };

struct RuleData;
typedef QSharedPointer<RuleData> RulePtr;
typedef QWeakPointer<RuleData> WeakRulePtr;

struct RuleData {
    bool visited;
    RuleData() : visited(false) {}

    QString name;
    QString contentName;
    QString include;
    QString beginPattern;
    QString endPattern;
    QString matchPattern;
    Regex begin;
    Regex match;
    QMap<int, RulePtr> captures;
    QMap<int, RulePtr> beginCaptures;
    QMap<int, RulePtr> endCaptures;
    QList<WeakRulePtr> patterns;
};

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

QString formatEndPattern(const QString& fmt, const Match& beginMatch) {
    int size = qMin(beginMatch.size(), 10);
    QString result = fmt;
    for (int i = 0; i < size; i++) {
        QString ref = "\\" + QString::number(i);
        result.replace(ref, beginMatch.cap(i));
    }
    return result;
}

QColor parseThemeColor(const QString& hex)
{
    QRegExp exp("#([\\w\\d]{6})([\\w\\d]{2})");
    if (exp.exactMatch(hex)) {
        bool ok = false;
        QString rgbahex = exp.cap(2) + exp.cap(1);
        QRgb rgba = rgbahex.toUInt(&ok, 16);
        if (ok) {
            return QColor::fromRgba(rgba);
        }
    }
    return QColor(hex);
}

bool listComparePrefix(const QStringList& list, const QStringList& prefix)
{
    if (list.length() < prefix.length())
        return false;

    for (int i = 0; i < prefix.length(); i++) {
        if (list[i] != prefix[i])
            return false;
    }
    return true;
}
}

class ScopeSelector : public QStack<QStringList>
{
public:
    ScopeSelector(const QStack<QString>& other)
    {
        QVectorIterator<QString> it(other);
        while (it.hasNext()) {
            QString element = it.next();
            if (!element.isEmpty()) {
                push(element.split("."));
            }
        }
    }

    ScopeSelector(const QString& selector)
    {
        if (selector.isEmpty())
            return;

        QStringListIterator it(selector.simplified().split(" "));
        while (it.hasNext()) {
            push(it.next().split("."));
        }
    }

    bool matches(const ScopeSelector& scope);
};

bool ScopeSelector::matches(const ScopeSelector& scope)
{
    int l = scope.size();
    for (int i = 0; i < size(); i++) {
        const QStringList& s = at(size() - 1 - i);
        while (true) {
            if (l == 0)
                return false;
            const QStringList& x = scope[--l];
            if (listComparePrefix(x, s))
                break;
        }
    }
    return true;
}

bool operator<(const ScopeSelector& lhs, const ScopeSelector& rhs)
{
    if (lhs == rhs)
        return false;

    int size = qMax(lhs.size(), rhs.size());
    for (int i = 0; i < size; i++) {
        if (i >= lhs.size())
            return false;
        if (i >= rhs.size())
            return true;
        QStringList l = lhs[lhs.size() - 1 - i];
        QStringList r = rhs[rhs.size() - 1 - i];
        if (l != r) {
            int sz = qMax(l.size(), r.size());
            for (int j = 0; j < sz; j++) {
                if (j >= l.size())
                    return false;
                if (j >= r.size())
                    return true;
                if (l[j] != r[j])
                    return l[j] < r[j];
            }
            Q_ASSERT(false);
        }
    }
    return false;
}

class ThemePrivate
{
    friend class Theme;

    QMap<ScopeSelector, QTextCharFormat> data;
};

Theme::Theme() :
    d(new ThemePrivate)
{
}

Theme::~Theme()
{
}

void Theme::setThemeData(const QVariantMap& themeData)
{
    d->data.clear();
    QVariantList settingsListData = themeData.value("settings").toList();
    QListIterator<QVariant> iter(settingsListData);
    while (iter.hasNext()) {
        QVariantMap itemData = iter.next().toMap();
        QTextCharFormat format;
        QVariantMap settingsData = itemData.value("settings").toMap();
        QMapIterator<QString, QVariant> settingsIter(settingsData);
        while (settingsIter.hasNext()) {
            settingsIter.next();
            QString key = settingsIter.key();
            if (key == "foreground") {
                QString hex = settingsIter.value().toString();
                format.setForeground(parseThemeColor(hex));
            } else if (key == "background") {
                QString hex = settingsIter.value().toString();
                format.setBackground(parseThemeColor(hex));
            } else if (key == "fontStyle") {
                QString styles = settingsIter.value().toString();
                QStringList list = styles.split(" ", QString::SkipEmptyParts);
                foreach (const QString& style, list) {
                    if (style == "bold") {
                        format.setFontWeight(75);
                    } else if (style == "italic") {
                        format.setFontItalic(true);
                    } else if (style == "underline") {
                        format.setFontUnderline(true);
                    } else {
                        qDebug() << "Unknown font style:" << style;
                    }
                }
            } else if (key == "caret") {
                QString color = settingsIter.value().toString();
                format.setProperty(QTextFormat::UserProperty, QBrush(parseThemeColor(color)));
            } else {
                qDebug() << "Unknown key in theme:" << key << "=>" << settingsIter.value();
            }
        }
        // XXX What to do when scope is not given?
        if (itemData.contains("name") && !itemData.contains("scope"))
            continue;
        QString scopes = itemData.value("scope").toString();
        foreach (QString scope, scopes.split(",")) {
            d->data[scope.trimmed()] = format;
        }
    }
}

QTextCharFormat Theme::format(const QString& name) const
{
    return d->data.value(name);
}

QTextCharFormat Theme::findFormat(const ScopeSelector& scope) const
{
    QTextCharFormat format;
    QMap<ScopeSelector, QTextCharFormat>::const_iterator it;
    for (it = d->data.begin(); it != d->data.end(); ++it) {
        ScopeSelector selector = it.key();
        if (selector.matches(scope)) {
            QTextCharFormat old = format;
            format = it.value();
            format.merge(old);
        }
    }
    return format;
}

class GrammarPrivate
{
    friend class Grammar;
    friend class Highlighter;
    friend class HighlighterPrivate;

    RulePtr root;
    QList<RulePtr> all;
    QMap<QString, RulePtr> repository;

    QMap<int, RulePtr> makeCaptures(const QVariantMap& capturesData);
    RulePtr makeRule(const QVariantMap& ruleData);
    QList<WeakRulePtr> makeRuleList(const QVariantList& ruleListData);
};

Grammar::Grammar() :
    d(new GrammarPrivate)
{
}

Grammar::~Grammar()
{
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
    QMap<QString, Grammar> grammars;

    Theme theme;

    void resolveChildRules(RulePtr parentRule, const Grammar& grammar);

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
    this->all.append(rule);

    rule->name = ruleData.value("name").toString();
    if (ruleData.contains("contentName"))
        rule->contentName = ruleData.value("contentName").toString();
    else
        rule->contentName = rule->name;
    rule->include = ruleData.value("include").toString();
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

QList<WeakRulePtr> GrammarPrivate::makeRuleList(const QVariantList& ruleListData)
{
    QList<WeakRulePtr> rules;
    QListIterator<QVariant> iter(ruleListData);
    while (iter.hasNext()) {
        QVariantMap ruleData = iter.next().toMap();
        rules << makeRule(ruleData);
    }
    return rules;
}

void HighlighterPrivate::resolveChildRules(RulePtr parentRule, const Grammar& grammar)
{
    if (parentRule->visited) return;
    parentRule->visited = true;

    QMutableListIterator<WeakRulePtr> iter(parentRule->patterns);
    while (iter.hasNext()) {
        RulePtr rule = iter.next();
        if (rule->include != QString()) {
            if (rule->include == "$base") {
                iter.setValue(this->root);
            } else if (rule->include == "$self") {
                iter.setValue(grammar.d->root);
            } else if (grammar.d->repository.contains(rule->include)) {
                rule = grammar.d->repository.value(rule->include);
                iter.setValue(rule);
                resolveChildRules(rule, grammar);
            } else if (grammars.contains(rule->include)) {
                iter.setValue(grammars.value(rule->include).d->root);
            } else {
                QVariantMap data = bundleManager->getSyntaxData(rule->include);
                if (!data.isEmpty()) {
                    Grammar g;
                    g.readSyntaxData(data);
                    grammars[rule->include] = g;
                    rule = g.d->root;
                    iter.setValue(rule);
                    resolveChildRules(rule, g);
                } else {
                    qWarning() << "Pattern not in repository" << rule->include;
                    iter.remove();
                }
            }
        } else {
            resolveChildRules(rule, grammar);
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
    d->grammar.readSyntaxData(d->bundleManager->getSyntaxData(scopeName));
    d->root = d->grammar.d->root;
    d->resolveChildRules(d->root, d->grammar);
}

void HighlighterPrivate::searchPatterns(const RulePtr& parentRule, const iter_t index, const iter_t end, const iter_t base,
                                        RulePtr& foundRule, MatchType& foundMatchType, Match& foundMatch)
{
    const int offset = index - base;

    foreach (RulePtr rule, parentRule->patterns) {
        if (rule->begin.isValid()) {
            Match match;
            if (rule->begin.search(base, end, index, end, match)) {
                if (foundMatch.isEmpty() || match.pos() < foundMatch.pos()) {
                    foundRule = rule;
                    foundMatchType = Begin;
                    foundMatch.swap(match);
                }
            }
        } else if (rule->match.isValid()) {
            Match match;
            if (rule->match.search(base, end, index, end, match)) {
                if (foundMatch.isEmpty() || match.pos() < foundMatch.pos()) {
                    foundRule = rule;
                    foundMatchType = Normal;
                    foundMatch.swap(match);
                }
            }
        } else {
            searchPatterns(rule, index, end, base, foundRule, foundMatchType, foundMatch);
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
    if (!d->root)
        return;

    QStack<ContextItem> contextStack;
    QStack<QString> scope;
    QTextBlock prevBlock = currentBlock().previous();
    if (prevBlock.userData()) {
        Context* ctx = static_cast<Context*>(prevBlock.userData());
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
            item.formattedEndPattern = formatEndPattern(foundRule->endPattern, foundMatch);
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
