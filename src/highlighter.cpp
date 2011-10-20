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
struct Node;
typedef QExplicitlySharedDataPointer<Node> Nodeptr;

struct Node : public QSharedData {
    QString name;
    QString include;
    boost::wregex begin;
    boost::wregex end;
    boost::wregex match;
    QMap<int, Nodeptr> captures;
    QMap<int, Nodeptr> beginCaptures;
    QMap<int, Nodeptr> endCaptures;
    QList<Nodeptr> patterns;
    QTextCharFormat format;
};
}

class HighlighterPrivate
{
    friend class Highlighter;

    Nodeptr rootPattern;

    QMap<QString, Nodeptr> repository;

    QMap<QString, QTextCharFormat> theme;

    QTextCharFormat makeFormat(const QString& name, const QTextCharFormat& baseFormat = QTextCharFormat());
    QMap<int, Nodeptr> makeCaptures(const QVariantMap& capturesData, const QTextCharFormat& baseFormat = QTextCharFormat());
    Nodeptr makePattern(const QVariantMap& patternData, const QTextCharFormat& baseFormat = QTextCharFormat());
    QList<Nodeptr> makePatternList(const QVariantList& patternListData);
    void resolveChildPatterns(Nodeptr rootPattern);
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

QMap<int, Nodeptr> HighlighterPrivate::makeCaptures(const QVariantMap& capturesData, const QTextCharFormat& baseFormat)
{
    QMap<int, Nodeptr> captures;
    QMapIterator<QString, QVariant> iter(capturesData);
    while (iter.hasNext()) {
        iter.next();
        int num = iter.key().toInt();
        QVariantMap data = iter.value().toMap();
        captures[num] = makePattern(data, baseFormat);
    }
    return captures;
}

Nodeptr HighlighterPrivate::makePattern(const QVariantMap& patternData, const QTextCharFormat& baseFormat)
{
    Nodeptr node(new Node);
    node->name = patternData.value("name").toString();
    node->include = patternData.value("include").toString();
    if (patternData.contains("begin"))
        node->begin.set_expression(patternData.value("begin").toString().toStdWString());
    if (patternData.contains("end"))
        node->end.set_expression(patternData.value("end").toString().toStdWString());
    if (patternData.contains("match"))
        node->match.set_expression(patternData.value("match").toString().toStdWString());
    QVariant patternListData = patternData.value("patterns");
    if (patternListData.isValid()) {
        node->patterns = makePatternList(patternListData.toList());
    }

    node->format = makeFormat(node->name, baseFormat);

    QVariant capturesData = patternData.value("captures");
    node->captures = makeCaptures(capturesData.toMap(), node->format);
    QVariant beginCapturesData = patternData.value("beginCaptures");
    if (beginCapturesData.isValid())
        node->beginCaptures = makeCaptures(beginCapturesData.toMap(), node->format);
    else
        node->beginCaptures = node->captures;
    QVariant endCapturesData = patternData.value("endCaptures");
    if (endCapturesData.isValid())
        node->endCaptures = makeCaptures(endCapturesData.toMap(), node->format);
    else
        node->endCaptures = node->captures;

    return node;
}

QList<Nodeptr> HighlighterPrivate::makePatternList(const QVariantList& patternListData)
{
    QList<Nodeptr> patterns;
    QListIterator<QVariant> iter(patternListData);
    while (iter.hasNext()) {
        QVariantMap patternData = iter.next().toMap();
        patterns << makePattern(patternData, theme.value(""));
    }
    return patterns;
}

void HighlighterPrivate::resolveChildPatterns(Nodeptr rootPattern)
{
    QList<Nodeptr>& patterns = rootPattern->patterns;

    QMutableListIterator<Nodeptr> iter(patterns);
    iter.toBack();
    while (iter.hasPrevious()) {
        Nodeptr& pattern = iter.previous();
        if (pattern->include != QString()) {
            if (this->repository.contains(pattern->include)) {
                iter.setValue(this->repository.value(pattern->include));
            } else {
                qWarning() << "Pattern not in repository" << pattern->include;
            }
        }
        if (pattern->match.empty() && pattern->begin.empty()) {
            QList<Nodeptr> childPatterns = pattern->patterns;
            iter.remove();
            foreach (Nodeptr childPattern, childPatterns) {
                iter.insert(childPattern);
            }
        } else {
            resolveChildPatterns(pattern);
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
        QVariantMap patternData = iter.value().toMap();
        d->repository["#" + iter.key()] = d->makePattern(patternData);
    }

    QVariantMap rootPatternData;
    rootPatternData["patterns"] = def.value("patterns");
    d->rootPattern = d->makePattern(rootPatternData);
    d->resolveChildPatterns(d->rootPattern);
}

void Highlighter::highlightBlock(const QString &text)
{
    enum MatchType { Normal, Begin, End };

    if (!d->rootPattern)
        return;

    QStack<Nodeptr> contextStack;
    contextStack.push(d->rootPattern);

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
    while (index != end) {
        Q_ASSERT(contextStack.size() > 0);
        Nodeptr context = contextStack.top();

        const diff_t offset = index - base;
        diff_t foundPos = _text.length();

        Nodeptr foundPattern;
        MatchType foundMatchType = Normal;
        boost::wsmatch foundMatch;

        if (!context->end.empty()) {
            Nodeptr pattern = context;
            boost::wsmatch match;
            if (boost::regex_search(index, end, match, pattern->end, flags, base)) {
                diff_t pos = offset + match.position();
                if (pos < foundPos) {
                    foundPos = pos;
                    foundPattern = pattern;
                    foundMatchType = End;
                    foundMatch.swap(match);
                }
            }
        }

        foreach (Nodeptr pattern, context->patterns) {
            if (!pattern->begin.empty()) {
                boost::wsmatch match;
                if (boost::regex_search(index, end, match, pattern->begin, flags, base)) {
                    diff_t pos = offset + match.position();
                    if (pos < foundPos) {
                        foundPos = pos;
                        foundPattern = pattern;
                        foundMatchType = Begin;
                        foundMatch.swap(match);
                    }
                }
            } else if (!pattern->match.empty()) {
                boost::wsmatch match;
                if (boost::regex_search(index, end, match, pattern->match, flags, base)) {
                    diff_t pos = offset + match.position();
                    if (pos < foundPos) {
                        foundPos = pos;
                        foundPattern = pattern;
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
            setFormat(offset, foundPos, context->format);
        }

        // Did we find anything to highlight?
        if (foundMatch.empty())
            break;

        Q_ASSERT(base + foundPos < end);
        setFormat(foundPos, foundMatch.length(), foundPattern->format);

        QMap<int, Nodeptr> captures;

        switch (foundMatchType) {
        case Normal:
            captures = foundPattern->captures;
            break;
        case Begin:
            captures = foundPattern->beginCaptures;
            contextStack.push(foundPattern);
            break;
        case End:
            captures = foundPattern->endCaptures;
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
            Nodeptr context = contextStack.pop();
            int range = contextStack.top()->patterns.size() + 1;
            int index = contextStack.top()->patterns.indexOf(context);
            Q_ASSERT(index != -1);
            state *= range;
            state += (index + 1);
            Q_ASSERT(state < 1UL << 31);
            qDebug() << state;
        }
        setCurrentBlockState(static_cast<int>(state));
    }
}
