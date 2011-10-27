#include "regex.h"

#include <oniguruma.h>

inline const OnigUChar* uc(Regex::iterator p)
{
    return reinterpret_cast<const OnigUChar*>(p);
}

class MatchPrivate
{
    friend class Regex;
    friend class Match;

    OnigRegion* region;
};

Match::Match() :
    d_ptr(new MatchPrivate)
{
    Q_D(Match);
    d->region = onig_region_new();
}

Match::~Match()
{
    Q_D(Match);
    onig_region_free(d->region, 1);
}

void Match::swap(Match &other)
{
    d_ptr.swap(other.d_ptr);
}

bool Match::isEmpty() const
{
    return d_func()->region->num_regs == 0;
}

int Match::size() const
{
    return d_func()->region->num_regs;
}

int Match::pos(int n) const
{
    return d_func()->region->beg[n]/sizeof(QChar);
}

int Match::len(int n) const
{
    return d_func()->region->end[n]/sizeof(QChar) - pos(n);
}

class RegexPrivate
{
public:
    RegexPrivate() : rx(0) {}
    ~RegexPrivate() { onig_free(rx); }

    OnigRegex rx;
    QString pattern;
};

Regex::Regex() :
    d_ptr(new RegexPrivate)
{
}

Regex::Regex(const QString& pattern) :
    d_ptr(new RegexPrivate)
{
    Q_D(Regex);
    d->pattern = pattern;

    OnigErrorInfo einfo;
    int r = onig_new(&d->rx, uc(pattern.begin()), uc(pattern.end()), ONIG_OPTION_NONE, ONIG_ENCODING_UTF16_LE, ONIG_SYNTAX_DEFAULT, &einfo);

    Q_ASSERT(r == ONIG_NORMAL);
}

Regex::~Regex()
{
}

bool Regex::isValid() const
{
    return d_func()->rx != 0;
}

QString Regex::pattern() const
{
    return d_func()->pattern;
}

bool Regex::search(const QString &target, Match &match)
{
    return search(target.begin(), target.end(), match);
}

bool Regex::search(iterator begin, iterator end, Match &match)
{
    return search(begin, end, begin, end, match);
}

bool Regex::search(iterator begin, iterator end, iterator offset, iterator range, Match &match)
{
    Q_D(Regex);
    int r = onig_search(d->rx, uc(begin), uc(end), uc(offset), uc(range), match.d_func()->region, ONIG_OPTION_NONE);
    return (r != ONIG_MISMATCH);
}
