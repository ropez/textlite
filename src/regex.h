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

    /**
      * Create an invalid regex
      */
    Regex();

    /**
      * Compile the given pattern into a regex.
      *
      * If the pattern is not recognized as a regular expression, isValid()
      * is false, and error() provides additional details.
      */
    Regex(const QString& pattern);

    /**
      * Destruction
      */
    ~Regex();

    /**
      * Returns true if this represents a valid regex.
      *
      * False if no pattern has been set, or if the pattern was not recognized.
      * In the latter case, error() provides additional information.
      */
    bool isValid() const;

    /**
      * If a pattern was set, and isValid() is false, an error message is
      * available.
      */
    QString error() const;

    /**
      * Return the pattern as a string
      */
    QString pattern() const;

    /**
      * Returns isValid()
      */
    bool setPattern(const QString& pattern);

    bool search(const QString& target, Match& match);
    bool search(iterator begin, iterator end, Match& match);
    bool search(iterator begin, iterator end, iterator offset, iterator range, Match& match);

private:
    Q_DECLARE_PRIVATE(Regex)
    QSharedPointer<RegexPrivate> d_ptr;
};

#endif // REGEX_H
