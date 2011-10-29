#include "highlighter.h"
#include "plistreader.h"

#include <QTextCharFormat>
#include <QRegExp>
#include <QList>
#include <QStack>
#include <QMap>

#include <QFile>

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
    QTextCharFormat format;
    QTextCharFormat contentFormat;
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
}

class HighlighterPrivate
{
    friend class Highlighter;

    RulePtr root;

    QList<RulePtr> all;
    QMap<QString, RulePtr> repository;

    QMap<QString, QTextCharFormat> theme;

    QTextCharFormat makeFormat(const QString& name, const QTextCharFormat& baseFormat = QTextCharFormat());
    QMap<int, RulePtr> makeCaptures(const QVariantMap& capturesData, const QTextCharFormat& baseFormat = QTextCharFormat());
    RulePtr makeRule(const QVariantMap& ruleData, const QTextCharFormat& baseFormat = QTextCharFormat());
    QList<WeakRulePtr> makeRuleList(const QVariantList& ruleListData);
    void resolveChildRules(RulePtr parentRule);

    void searchPatterns(const RulePtr& parentRule, const iter_t index, const iter_t end, const iter_t base,
                        int& foundPos, RulePtr& foundRule, MatchType& foundMatchType, Match& foundMatch);
};

QTextCharFormat HighlighterPrivate::makeFormat(const QString& name, const QTextCharFormat& baseFormat)
{
    QTextCharFormat format = baseFormat;
    QMapIterator<QString, QTextCharFormat> iter(theme);
    while (iter.hasNext()) {
        iter.next();
        QString scope = iter.key();
        if (!scope.isEmpty() && name.startsWith(scope)) {
            format.merge(iter.value());
        }
    }
    return format;
}

QMap<int, RulePtr> HighlighterPrivate::makeCaptures(const QVariantMap& capturesData, const QTextCharFormat& baseFormat)
{
    QMap<int, RulePtr> captures;
    QMapIterator<QString, QVariant> iter(capturesData);
    while (iter.hasNext()) {
        iter.next();
        int num = iter.key().toInt();
        QVariantMap data = iter.value().toMap();
        captures[num] = makeRule(data, baseFormat);
    }
    return captures;
}

RulePtr HighlighterPrivate::makeRule(const QVariantMap& ruleData, const QTextCharFormat& baseFormat)
{
    RulePtr rule(new RuleData);
    this->all.append(rule);

    rule->name = ruleData.value("name").toString();
    rule->contentName = ruleData.value("contentName").toString();
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

    rule->format = makeFormat(rule->name, baseFormat);
    rule->contentFormat = makeFormat(rule->contentName, rule->format);

    QVariant capturesData = ruleData.value("captures");
    rule->captures = makeCaptures(capturesData.toMap(), rule->format);
    QVariant beginCapturesData = ruleData.value("beginCaptures");
    if (beginCapturesData.isValid())
        rule->beginCaptures = makeCaptures(beginCapturesData.toMap(), rule->format);
    else
        rule->beginCaptures = rule->captures;
    QVariant endCapturesData = ruleData.value("endCaptures");
    if (endCapturesData.isValid())
        rule->endCaptures = makeCaptures(endCapturesData.toMap(), rule->format);
    else
        rule->endCaptures = rule->captures;

    return rule;
}

QList<WeakRulePtr> HighlighterPrivate::makeRuleList(const QVariantList& ruleListData)
{
    QList<WeakRulePtr> rules;
    QListIterator<QVariant> iter(ruleListData);
    while (iter.hasNext()) {
        QVariantMap ruleData = iter.next().toMap();
        rules << makeRule(ruleData, theme.value(""));
    }
    return rules;
}

void HighlighterPrivate::resolveChildRules(RulePtr parentRule)
{
    if (parentRule->visited) return;
    parentRule->visited = true;

    QMutableListIterator<WeakRulePtr> iter(parentRule->patterns);
    while (iter.hasNext()) {
        RulePtr rule = iter.next();
        if (rule->include != QString()) {
            if (rule->include == "$self") {
                rule = this->root;
                iter.setValue(rule);
            } else if (this->repository.contains(rule->include)) {
                rule = this->repository.value(rule->include);
                iter.setValue(rule);
            } else {
                qWarning() << "Pattern not in repository" << rule->include;
                iter.remove();
                continue;
            }
        }
        resolveChildRules(rule);
    }
}

Highlighter::Highlighter(QTextDocument* document) :
    QSyntaxHighlighter(document),
    d(new HighlighterPrivate)
{
}

Highlighter::~Highlighter()
{
    delete d;
}

void Highlighter::readThemeFile(const QString& themeFile)
{
    PlistReader reader;
    QVariantMap def = reader.read(themeFile).toMap();
    QVariantList settingsListData = def.value("settings").toList();
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
                QString color = settingsIter.value().toString();
                format.setForeground(QColor(color));
            } else if (key == "background") {
                QString color = settingsIter.value().toString();
                format.setBackground(QColor(color));
            } else if (key == "fontStyle") {
                QString style = settingsIter.value().toString();
                if (style.isEmpty()) {
                    format.setFontWeight(50);
                } else if (style == "bold") {
                    format.setFontWeight(75);
                } else if (style == "italic") {
                    format.setFontItalic(true);
                } else {
                    qDebug() << "Unknown font style:" << style;
                }
            } else {
                qDebug() << "Unknown key in theme:" << key << "=>" << settingsIter.value();
            }
        }
        QString scopes = itemData.value("scope").toString();
        foreach (QString scope, scopes.split(",")) {
            d->theme[scope.trimmed()] = format;
        }
    }
}

void Highlighter::readSyntaxFile(const QString& syntaxFile)
{
    PlistReader reader;
    QVariantMap def = reader.read(syntaxFile).toMap();
    QVariantMap repositoryData = def.value("repository").toMap();

    QMapIterator<QString, QVariant> iter(repositoryData);
    while (iter.hasNext()) {
        iter.next();
        QVariantMap ruleData = iter.value().toMap();
        d->repository["#" + iter.key()] = d->makeRule(ruleData);
    }

    QVariantMap rootData;
    rootData["patterns"] = def.value("patterns");
    d->root = d->makeRule(rootData);
    d->resolveChildRules(d->root);
}

void HighlighterPrivate::searchPatterns(const RulePtr& parentRule, const iter_t index, const iter_t end, const iter_t base,
                                        int& foundPos, RulePtr& foundRule, MatchType& foundMatchType, Match& foundMatch)
{
    const int offset = index - base;

    foreach (RulePtr rule, parentRule->patterns) {
        if (rule->begin.isValid()) {
            Match match;
            if (rule->begin.search(base, end, index, end, match)) {
                if (foundMatch.isEmpty() || match.pos() < foundPos) {
                    foundPos = match.pos();
                    foundRule = rule;
                    foundMatchType = Begin;
                    foundMatch.swap(match);
                }
            }
        } else if (rule->match.isValid()) {
            Match match;
            if (rule->match.search(base, end, index, end, match)) {
                if (foundMatch.isEmpty() || match.pos() < foundPos) {
                    foundPos = match.pos();
                    foundRule = rule;
                    foundMatchType = Normal;
                    foundMatch.swap(match);
                }
            }
        } else {
            searchPatterns(rule, index, end, base, foundPos, foundRule, foundMatchType, foundMatch);
        }
        Q_ASSERT(foundPos >= offset);
        if (foundPos == offset)
            break; // Don't need to continue
    }
}

void Highlighter::highlightBlock(const QString &text)
{
    if (!d->root)
        return;

    QStack<ContextItem> contextStack;
    QTextBlock prevBlock = currentBlock().previous();
    if (prevBlock.userData()) {
        Context* ctx = static_cast<Context*>(prevBlock.userData());
        contextStack = ctx->stack;
    } else {
        contextStack.push(ContextItem(d->root));
    }

    const iter_t base = text.begin();
    const iter_t end = text.end();

    iter_t index = base;
    while (true) {
        Q_ASSERT(contextStack.size() > 0);
        ContextItem& context = contextStack.top();

        const int offset = index - base;
        int foundPos = text.length();

        RulePtr foundRule;
        MatchType foundMatchType = Normal;
        Match foundMatch;

        if (context.end.isValid()) {
            RulePtr rule = context.rule;
            Match match;
            if (context.end.search(base, end, index, end, match)) {
                foundPos = match.pos();
                foundRule = rule;
                foundMatchType = End;
                foundMatch.swap(match);
            }
        }

        d->searchPatterns(context.rule, index, end, base, foundPos, foundRule, foundMatchType, foundMatch);

        // Highlight skipped section
        if (foundPos != offset) {
            setFormat(offset, foundPos - offset, context.rule->contentFormat);
        }

        // Did we find anything to highlight?
        if (foundMatch.isEmpty())
            break;

        Q_ASSERT(base + foundPos <= end);
        setFormat(foundMatch.pos(), foundMatch.len(), foundRule->format);

        QMap<int, RulePtr> captures;

        switch (foundMatchType) {
        case Normal:
            captures = foundRule->captures;
            break;
        case Begin:
        {
            captures = foundRule->beginCaptures;
            // Compile the regular expression that will end this context,
            // and may include captures from the found match
            ContextItem item(foundRule);
            item.formattedEndPattern = formatEndPattern(foundRule->endPattern, foundMatch);
            item.end.setPattern(item.formattedEndPattern);
            contextStack.push(item);
            break;
        }
        case End:
            captures = foundRule->endCaptures;
            contextStack.pop();
            break;
        }

        // Highlight captures
        for (int c = 1; c < foundMatch.size(); c++) {
            if (foundMatch.matched(c)) {
                if (captures.contains(c)) {
                    Q_ASSERT(foundMatch.pos(c) >= foundMatch.pos());
                    Q_ASSERT(foundMatch.pos(c) + foundMatch.len(c) <= foundMatch.pos() + foundMatch.len());
                    setFormat(foundMatch.pos(c), foundMatch.len(c), captures[c]->format);
                }
            }
        }
        index = base + foundMatch.pos() + foundMatch.len();
    }

    if (contextStack.size() > 1) {
        Context* ctx = new Context;
        ctx->stack = contextStack;
        setCurrentBlockUserData(ctx);
        setCurrentBlockState(_Hash(ctx->stack));
    } else {
        setCurrentBlockUserData(0);
        setCurrentBlockState(-1);
    }
}
