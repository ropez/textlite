#ifndef REGEX_H
#define REGEX_H

#include <QString>
#include <QScopedPointer>
#include <QSharedPointer>

class MatchPrivate;

class Match
{
public:
    Match();
    ~Match();

    void swap(Match& other);

    bool isEmpty() const;
    int size() const;

    int pos(int n = 0) const;
    int len(int n = 0) const;

private:
    friend class Regex;

    Q_DISABLE_COPY(Match)
    Q_DECLARE_PRIVATE(Match)
    QScopedPointer<MatchPrivate> d_ptr;
};

class RegexPrivate;

class Regex
{
public:
    typedef QString::const_iterator iterator;

    Regex();
    Regex(const QString& pattern);
    ~Regex();

    bool isValid() const;
    QString error() const;

    QString pattern() const;

    bool setPattern(const QString& pattern);

    bool search(const QString& target, Match& match);
    bool search(iterator begin, iterator end, Match& match);
    bool search(iterator begin, iterator end, iterator offset, iterator range, Match& match);

private:
    Q_DECLARE_PRIVATE(Regex)
    QSharedPointer<RegexPrivate> d_ptr;
};

#endif // REGEX_H
