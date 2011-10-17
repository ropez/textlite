#include "highlighter.h"
#include "plistreader.h"

#include <QTextCharFormat>
#include <QRegExp>
#include <QList>
#include <QMap>

#include <QFile>

#include <QtDebug>

namespace {
struct Node;
typedef QExplicitlySharedDataPointer<Node> Nodeptr;

struct Node : public QSharedData {
    QString name;
    QString include;
    QString begin;
    QString end;
    QString match;
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
    node->begin = patternData.value("begin").toString();
    node->end = patternData.value("end").toString();
    node->match = patternData.value("match").toString();
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
        if (pattern->match == QString() && pattern->begin == QString()) {
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

void Highlighter::readThemeFile(const QByteArray& themeFile)
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

void Highlighter::readSyntaxFile(const QByteArray& syntaxFile)
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

    int index = 0;
    while (index < text.length()) {
        Nodeptr context = contextStack.last();

        int foundPos = text.length();
        int foundLength = 0;

        Nodeptr foundPattern;
        MatchType foundMatchType;

        if (context->end != QString()) {
            Nodeptr pattern = context;
            QRegExp exp(pattern->end);
            int pos = exp.indexIn(text, index);
            if (pos != -1 && pos < foundPos) {
                foundPos = pos;
                foundLength = exp.matchedLength();
                foundPattern = pattern;
                foundMatchType = End;
            }
        }

        foreach (Nodeptr pattern, context->patterns) {
            if (pattern->begin != QString()) {
                QRegExp exp(pattern->begin);
                int pos = exp.indexIn(text, index);
                if (pos != -1 && pos < foundPos) {
                    foundPos = pos;
                    foundLength = exp.matchedLength();
                    foundPattern = pattern;
                    foundMatchType = Begin;
                }
            } else if (pattern->match != QString()) {
                QRegExp exp(pattern->match);
                int pos = exp.indexIn(text, index);
                if (pos != -1 && pos < foundPos) {
                    foundPos = pos;
                    foundLength = exp.matchedLength();
                    foundPattern = pattern;
                    foundMatchType = Normal;
                }
            }
            Q_ASSERT(foundPos >= index);
            if (foundPos == index)
                break; // Don't need to continue
        }

        // Highlight skipped section
        if (foundPos != index) {
            setFormat(index, foundPos, context->format);
        }

        // Did we find anything to highlight?
        if (foundLength > 0) {
            Q_ASSERT(foundPos < text.length());
            setFormat(foundPos, foundLength, foundPattern->format);

            QRegExp exp;
            QMap<int, Nodeptr> captures;

            switch (foundMatchType) {
            case Normal:
                exp.setPattern(foundPattern->match);
                captures = foundPattern->captures;
                break;
            case Begin:
                exp.setPattern(foundPattern->begin);
                captures = foundPattern->beginCaptures;
                contextStack.append(foundPattern);
                break;
            case End:
                exp.setPattern(foundPattern->end);
                captures = foundPattern->endCaptures;
                contextStack.removeLast();
                break;
            }

            // Re-run regexp to get captures
            exp.indexIn(text, index);
            for (int c = 1; c <= exp.captureCount(); c++) {
                if (exp.pos(c) != -1) {
                    if (captures.contains(c)) {
                        Q_ASSERT(exp.pos(c) >= foundPos);
                        Q_ASSERT(exp.cap(c).length() > 0);
                        Q_ASSERT(exp.pos(c) + exp.cap(c).length() <= foundPos + foundLength);
                        setFormat(exp.pos(c), exp.cap(c).length(), captures[c]->format);
                    }
                }
            }
        }
        Q_ASSERT(index < foundPos + foundLength);
        index = foundPos + foundLength;
    }
}
