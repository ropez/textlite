#include "highlighter.h"
#include "plistreader.h"

#include <QTextCharFormat>
#include <QRegExp>
#include <QList>
#include <QMap>

#include <QFile>

#include <QtDebug>

class HighlighterPrivate
{
    friend class Highlighter;

    struct pattern {
        QString name;
        QString begin;
        QString end;
        QString match;
        QMap<int, pattern> captures;
        QMap<int, pattern> beginCaptures;
        QMap<int, pattern> endCaptures;
        QList<pattern> patterns;
        QTextCharFormat format;
    };
    pattern rootPattern;

    QMap<QString, pattern> repository;

    QMap<QString, QTextCharFormat> theme;

    QTextCharFormat makeFormat(const QString& name, const QTextCharFormat& baseFormat = QTextCharFormat());
    QMap<int, pattern> makeCaptures(const QVariantMap& capturesData, const QTextCharFormat& baseFormat = QTextCharFormat());
    pattern makePattern(const QVariantMap& patternData, const QTextCharFormat& baseFormat = QTextCharFormat());
    QList<pattern> makePatternList(const QVariantList& patternListData);
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

QMap<int, HighlighterPrivate::pattern> HighlighterPrivate::makeCaptures(const QVariantMap& capturesData, const QTextCharFormat& baseFormat)
{
    QMap<int, pattern> captures;
    QMapIterator<QString, QVariant> iter(capturesData);
    while (iter.hasNext()) {
        iter.next();
        int num = iter.key().toInt();
        QVariantMap data = iter.value().toMap();
        captures[num] = makePattern(data, baseFormat);
    }
    return captures;
}

HighlighterPrivate::pattern HighlighterPrivate::makePattern(const QVariantMap& patternData, const QTextCharFormat& baseFormat)
{
    if (patternData.contains("include")) {
        return this->repository[patternData.value("include").toString()];
    } else {
        HighlighterPrivate::pattern pattern;
        pattern.name = patternData.value("name").toString();
        pattern.begin = patternData.value("begin").toString();
        pattern.end = patternData.value("end").toString();
        pattern.match = patternData.value("match").toString();
        QVariant patternListData = patternData.value("patterns");
        if (patternListData.isValid()) {
            pattern.patterns = makePatternList(patternListData.toList());
        }

        pattern.format = makeFormat(pattern.name, baseFormat);

        QVariant capturesData = patternData.value("captures");
        pattern.captures = makeCaptures(capturesData.toMap(), pattern.format);
        QVariant beginCapturesData = patternData.value("beginCaptures");
        pattern.beginCaptures = makeCaptures(beginCapturesData.toMap(), pattern.format);
        QVariant endCapturesData = patternData.value("endCaptures");
        pattern.endCaptures = makeCaptures(endCapturesData.toMap(), pattern.format);

        return pattern;
    }
}

QList<HighlighterPrivate::pattern> HighlighterPrivate::makePatternList(const QVariantList& patternListData)
{
    QList<HighlighterPrivate::pattern> patterns;
    QListIterator<QVariant> iter(patternListData);
    while (iter.hasNext()) {
        QVariantMap patternData = iter.next().toMap();
        patterns << makePattern(patternData, theme.value(""));
    }
    return patterns;
}

Highlighter::Highlighter(QTextDocument* document) :
    QSyntaxHighlighter(document),
    d(new HighlighterPrivate)
{
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
}

void Highlighter::highlightBlock(const QString &text)
{
    QList<HighlighterPrivate::pattern> contextStack;
    contextStack.append(d->rootPattern);

    QTextCharFormat defaultFormat = d->theme[""];
    int index = 0;
    while (index < text.length()) {
        HighlighterPrivate::pattern context = contextStack.last();

        bool found = false;

        if (context.end != QString()) {
            HighlighterPrivate::pattern pattern = context;
            QRegExp exp("^" + pattern.end);
            if (exp.indexIn(text, index, QRegExp::CaretAtOffset) == index) {
                int length = exp.matchedLength();
                setFormat(index, length, pattern.format);
                for (int c = 1; c <= exp.captureCount(); c++) {
                    if (exp.pos(c) != -1) {
                        if (pattern.endCaptures.contains(c)) {
                            setFormat(exp.pos(c), exp.cap(c).length(), pattern.endCaptures[c].format);
                        } else if (pattern.captures.contains(c)) {
                            setFormat(exp.pos(c), exp.cap(c).length(), pattern.captures[c].format);
                        }
                    }
                }
                contextStack.removeLast();
                index += length;
                found = true;
                continue;
            }
        }

        foreach (HighlighterPrivate::pattern pattern, context.patterns) {
            if (pattern.name == QString())
                continue;
            if (pattern.begin != QString()) {
                QRegExp exp("^" + pattern.begin);
                if (exp.indexIn(text, index, QRegExp::CaretAtOffset) == index) {
                    int length = exp.matchedLength();
                    setFormat(index, length, pattern.format);
                    for (int c = 1; c <= exp.captureCount(); c++) {
                        if (exp.pos(c) != -1) {
                            if (pattern.beginCaptures.contains(c)) {
                                setFormat(exp.pos(c), exp.cap(c).length(), pattern.beginCaptures[c].format);
                            } else if (pattern.captures.contains(c)) {
                                setFormat(exp.pos(c), exp.cap(c).length(), pattern.captures[c].format);
                            }
                        }
                    }
                    contextStack.append(pattern);
                    index += length;
                    found = true;
                    break;
                } else {
                    continue;
                }
            } else if (pattern.match != QString()) {
                QRegExp exp("^" + pattern.match);
                if (exp.indexIn(text, index, QRegExp::CaretAtOffset) == index) {
                    int length = exp.matchedLength();
                    setFormat(index, length, pattern.format);
                    for (int c = 1; c <= exp.captureCount(); c++) {
                        if (exp.pos(c) != -1) {
                            if (pattern.captures.contains(c)) {
                                setFormat(exp.pos(c), exp.cap(c).length(), pattern.captures[c].format);
                            }
                        }
                    }
                    index += length;
                    found = true;
                    break;
                } else {
                    continue;
                }
            }
        }
        if (!found) {
            setFormat(index, 1, context.format);
            index++;
        }
    }
}
