#include "highlighter.h"
#include "plistreader.h"

#include <QTextCharFormat>
#include <QRegExp>
#include <QList>
#include <QStack>
#include <QMap>

#include <QFile>

#include <QtDebug>

#include <boost/regex.hpp>

namespace {
struct RuleData;
typedef QSharedPointer<RuleData> RulePtr;
typedef QWeakPointer<RuleData> WeakRulePtr;

struct RuleData {
    bool visited;
    RuleData() : visited(false) {}

    QString name;
    QString contentName;
    QString include;
    boost::wregex begin;
    boost::wregex end;
    boost::wregex match;
    QMap<int, RulePtr> captures;
    QMap<int, RulePtr> beginCaptures;
    QMap<int, RulePtr> endCaptures;
    QList<WeakRulePtr> patterns;
    QTextCharFormat format;
    QTextCharFormat contentFormat;
};
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
    if (ruleData.contains("begin"))
        rule->begin.set_expression(ruleData.value("begin").toString().toStdWString());
    if (ruleData.contains("end"))
        rule->end.set_expression(ruleData.value("end").toString().toStdWString());
    if (ruleData.contains("match"))
        rule->match.set_expression(ruleData.value("match").toString().toStdWString());
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
    iter.toBack();
    while (iter.hasPrevious()) {
        RulePtr rule = iter.previous();
        if (rule->include != QString()) {
            if (rule->include == "$self") {
                QList<WeakRulePtr> childPatterns = this->root->patterns;
                iter.remove();
                foreach (RulePtr childPattern, childPatterns) {
                    iter.insert(childPattern);
                }
                continue;
            } else if (this->repository.contains(rule->include)) {
                rule = this->repository.value(rule->include);
                iter.setValue(rule);
            } else {
                qWarning() << "Pattern not in repository" << rule->include;
                iter.remove();
            }
        }
        if (rule->match.empty() && rule->begin.empty()) {
            QList<WeakRulePtr> childPatterns = rule->patterns;
            iter.remove();
            foreach (RulePtr childPattern, childPatterns) {
                iter.insert(childPattern);
            }
        } else {
            resolveChildRules(rule);
        }
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

void Highlighter::highlightBlock(const QString &text)
{
    enum MatchType { Normal, Begin, End };

    if (!d->root)
        return;

    QStack<RulePtr> contextStack;
    contextStack.push(d->root);

    // restore encoded state
    {
        int state = previousBlockState();
        while (state > 0) {
            int range = contextStack.top()->patterns.size() + 1;
            div_t div = std::div(state, range);
            int index = div.rem - 1;
            contextStack.push(contextStack.top()->patterns[index]);
            state = div.quot;
        }
    }


    typedef std::wstring::const_iterator pos_t;
    typedef std::wstring::difference_type diff_t;

    const boost::match_flag_type flags = boost::match_default;
    const std::wstring _text = text.toStdWString();
    const pos_t base = _text.begin();
    const pos_t end = _text.end();

    pos_t index = base;
    while (true) {
        Q_ASSERT(contextStack.size() > 0);
        RulePtr context = contextStack.top();

        const diff_t offset = index - base;
        diff_t foundPos = _text.length();

        RulePtr foundRule;
        MatchType foundMatchType = Normal;
        boost::wsmatch foundMatch;

        if (!context->end.empty()) {
            RulePtr rule = context;
            boost::wsmatch match;
            if (boost::regex_search(index, end, match, rule->end, flags, base)) {
                diff_t pos = offset + match.position();
                if (foundMatch.empty() || pos < foundPos) {
                    foundPos = pos;
                    foundRule = rule;
                    foundMatchType = End;
                    foundMatch.swap(match);
                }
            }
        }

        foreach (RulePtr rule, context->patterns) {
            if (!rule->begin.empty()) {
                boost::wsmatch match;
                if (boost::regex_search(index, end, match, rule->begin, flags, base)) {
                    diff_t pos = offset + match.position();
                    if (foundMatch.empty() || pos < foundPos) {
                        foundPos = pos;
                        foundRule = rule;
                        foundMatchType = Begin;
                        foundMatch.swap(match);
                    }
                }
            } else if (!rule->match.empty()) {
                boost::wsmatch match;
                if (boost::regex_search(index, end, match, rule->match, flags, base)) {
                    diff_t pos = offset + match.position();
                    if (foundMatch.empty() || pos < foundPos) {
                        foundPos = pos;
                        foundRule = rule;
                        foundMatchType = Normal;
                        foundMatch.swap(match);
                    }
                }
            }
            Q_ASSERT(foundPos >= offset);
            if (foundPos == offset)
                break; // Don't need to continue
        }

        // Highlight skipped section
        if (foundPos != offset) {
            setFormat(offset, foundPos - offset, context->contentFormat);
        }

        // Did we find anything to highlight?
        if (foundMatch.empty())
            break;

        Q_ASSERT(base + foundPos <= end);
        setFormat(foundPos, foundMatch.length(), foundRule->format);

        QMap<int, RulePtr> captures;

        switch (foundMatchType) {
        case Normal:
            captures = foundRule->captures;
            break;
        case Begin:
            captures = foundRule->beginCaptures;
            contextStack.push(foundRule);
            break;
        case End:
            captures = foundRule->endCaptures;
            contextStack.pop();
            break;
        }

        // Highlight captures
        for (size_t c = 1; c <= foundMatch.size(); c++) {
            if (foundMatch.position(c) != -1) {
                diff_t pos = offset + foundMatch.position(c);
                if (captures.contains(c)) {
                    Q_ASSERT(pos >= foundPos);
                    Q_ASSERT(foundMatch.length(c) > 0);
                    Q_ASSERT(pos + foundMatch.length(c) <= foundPos + foundMatch.length());
                    setFormat(pos, foundMatch.length(c), captures[c]->format);
                }
            }
        }
        index = base + foundPos + foundMatch.length();
    }

    // encode and save state
    {
        qint64 state = 0;
        while (contextStack.size() > 1) {
            RulePtr context = contextStack.pop();
            int range = contextStack.top()->patterns.size() + 1;
            int index = contextStack.top()->patterns.indexOf(context);
            Q_ASSERT(index != -1);
            state *= range;
            state += (index + 1);
            Q_ASSERT(state < 1LL << 31);
        }
        setCurrentBlockState(static_cast<int>(state));
    }
}
