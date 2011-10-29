#ifndef REGEX_H
#define REGEX_H

#include <QString>
#include <QScopedPointer>
#include <QSharedPointer>

class MatchPrivate;
class RegexPrivate;

/**
  * Provides information about a successful search result.
  */
class Match
{
public:

    /**
      * Create an empty instance
      */
    Match();

    /**
      * Destruction
      */
    ~Match();

    /**
      * Swap the content of this object with other
      */
    void swap(Match& other);

    /**
      * Returns true if size() is 0
      */
    bool isEmpty() const;

    /**
      * Returns the number of matched subexpressions. 0 for newly created
      * instances.
      */
    int size() const;

    /**
      * Returns true if the nth subexpression was matched (n = 0 means the whole
      * expression)
      */
    bool matched(int n = 0) const;

    /**
      * Returns the position of the nth subexpression (n = 0 means the whole
      * expression)
      */
    int pos(int n = 0) const;

    /**
      * Returns the length of the nth subexpression (n = 0 means the whole
      * expression)
      */
    int len(int n = 0) const;

    /**
      * Returns the nth matched subexpression as a new string. Calling this is
      * valid as long as the begin argument passed to Regex::search() is still
      * valid. The returned string is a copy, which is valid forever.
      */
    QString cap(int n = 0) const;

private:
    friend class Regex;

    Q_DISABLE_COPY(Match)
    Q_DECLARE_PRIVATE(Match)
    QScopedPointer<MatchPrivate> d_ptr;
};

/**
  * Represents a compiled regular expression.
  */
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

    /**
      * Shortcut for search(target.begin(), target.end(), match)
      */
    bool search(const QString& target, Match& match);

    /**
      * Shortcut for search(begin, end, begin, end, match)
      */
    bool search(iterator begin, iterator end, Match& match);

    /**
      * Search for this regular expression within the given boundaries.
      *
      * If found, the results are stored in match, and the function returns
      * true. If not found, the function returns false, and match is undefined.
      *
      * The expression must be found within the boundaries of offset and range,
      * which must be a subregion of begin and end. The outer region, begin and
      * end, provides the boundaries for lookbehind, lookahead, line boundaries,
      * word boundaries and so on.
      */
    bool search(iterator begin, iterator end, iterator offset, iterator range, Match& match);

private:
    Q_DECLARE_PRIVATE(Regex)
    QSharedPointer<RegexPrivate> d_ptr;
};

#endif // REGEX_H
