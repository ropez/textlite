#include "highlighter.h"
#include "plistreader.h"

#include <QTextCharFormat>
#include <QRegExp>
#include <QList>
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
    std::wstring begin;
    std::wstring end;
    std::wstring match;
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
    node->begin = patternData.value("begin").toString().toStdWString();
    node->end = patternData.value("end").toString().toStdWString();
    node->match = patternData.value("match").toString().toStdWString();
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

    QList<Nodeptr> contextStack;
    contextStack.append(d->rootPattern);
    int state = previousBlockState();
    if (state != -1) {
        contextStack.append(d->rootPattern->patterns[state]);
    }


    typedef std::wstring::const_iterator pos_t;
    typedef std::wstring::difference_type diff_t;

    const boost::match_flag_type flags = boost::match_default;
    const std::wstring _text = text.toStdWString();
    const pos_t base = _text.begin();
    const pos_t end = _text.end();

    pos_t index = base;
    while (index != end) {
        Nodeptr context = contextStack.last();

        const diff_t offset = index - base;
        diff_t foundPos = _text.length();
        diff_t foundLength = 0;

        Nodeptr foundPattern;
        MatchType foundMatchType;

        if (!context->end.empty()) {
            Nodeptr pattern = context;
            boost::wregex exp(pattern->end);
            boost::wsmatch match;
            if (boost::regex_search(index, end, match, exp, flags, base)) {
                diff_t pos = offset + match.position();
                if (pos < foundPos) {
                    foundPos = pos;
                    foundLength = match.length();
                    foundPattern = pattern;
                    foundMatchType = End;
                }
            }
        }

        foreach (Nodeptr pattern, context->patterns) {
            if (!pattern->begin.empty()) {
                boost::wregex exp(pattern->begin);
                boost::wsmatch match;
                if (boost::regex_search(index, end, match, exp, flags, base)) {
                    diff_t pos = offset + match.position();
                    if (pos < foundPos) {
                        foundPos = pos;
                        foundLength = match.length();
                        foundPattern = pattern;
                        foundMatchType = Begin;
                    }
                }
            } else if (!pattern->match.empty()) {
                boost::wregex exp(pattern->match);
                boost::wsmatch match;
                if (boost::regex_search(index, end, match, exp, flags, base)) {
                    diff_t pos = offset + match.position();
                    if (pos < foundPos) {
                        foundPos = pos;
                        foundLength = match.length();
                        foundPattern = pattern;
                        foundMatchType = Normal;
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
        if (foundPos != text.length()) {
            Q_ASSERT(base + foundPos < end);
            setFormat(foundPos, foundLength, foundPattern->format);

            boost::wregex exp;
            QMap<int, Nodeptr> captures;

            switch (foundMatchType) {
            case Normal:
                exp.set_expression(foundPattern->match);
                captures = foundPattern->captures;
                break;
            case Begin:
                exp.set_expression(foundPattern->begin);
                captures = foundPattern->beginCaptures;
                contextStack.append(foundPattern);
                break;
            case End:
                exp.set_expression(foundPattern->end);
                captures = foundPattern->endCaptures;
                contextStack.removeLast();
                break;
            }

            // Re-run regexp to get captures
            boost::wsmatch match;
            boost::regex_search(index, end, match, exp, flags, base);
            for (size_t c = 1; c <= match.size(); c++) {
                if (match.position(c) != -1) {
                    diff_t pos = offset + match.position(c);
                    if (captures.contains(c)) {
                        Q_ASSERT(pos >= foundPos);
                        Q_ASSERT(match.length(c) > 0);
                        Q_ASSERT(pos + match.length(c) <= foundPos + foundLength);
                        setFormat(pos, match.length(c), captures[c]->format);
                    }
                }
            }
        }
        index = base + foundPos + foundLength;
    }
    if (contextStack.length() == 1) {
        setCurrentBlockState(-1);
    } else if (contextStack.length() == 2) {
        setCurrentBlockState(contextStack.first()->patterns.indexOf(contextStack.last()));
    } else {
        qWarning() << "Context too deep";
    }
}
