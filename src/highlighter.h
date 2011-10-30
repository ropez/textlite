#ifndef HIGHLIGHTER_H
#define HIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QScopedPointer>

class ThemeManagerPrivate;
class HighlighterPrivate;

class Theme
{
public:
    void readThemeFile(const QString& themeFile);

    QTextCharFormat format(const QString& name) const;
    QTextCharFormat mergeFormat(const QString& name, const QTextCharFormat& baseFormat) const;

private:
    QHash<QString, QTextCharFormat> data;
};

class ThemeManager : public QObject
{
    Q_OBJECT
public:
    explicit ThemeManager(const QString& themeDirPath, QObject* parent = 0);
    ~ThemeManager();

    Theme theme() const;

signals:
    void themeChanged(const Theme& theme);

public slots:
    void readThemeFile(const QString& themeFile);

private:
    QScopedPointer<ThemeManagerPrivate> d;
};

class Highlighter : public QSyntaxHighlighter
{
    Q_OBJECT
public:
    explicit Highlighter(QTextDocument *document, ThemeManager* themeManager);
    ~Highlighter();

public slots:
    void setTheme(const Theme& theme);
    void readSyntaxFile(const QString& syntaxFile);

protected:
    void highlightBlock(const QString &text);

private:
    HighlighterPrivate* d;
};

#endif // HIGHLIGHTER_H
