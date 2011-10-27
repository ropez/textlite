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
    d_func()->region = onig_region_new();
}

Match::~Match()
{
    onig_region_free(d_func()->region, 1);
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
    QString error;
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
    int r = onig_new(&d->rx, uc(pattern.begin()), uc(pattern.end()), ONIG_OPTION_CAPTURE_GROUP, ONIG_ENCODING_UTF16_LE, ONIG_SYNTAX_DEFAULT, &einfo);
    if (r != ONIG_NORMAL) {
        Q_ASSERT(d->rx == 0);
        QByteArray raw(ONIG_MAX_ERROR_MESSAGE_LEN, Qt::Uninitialized);
        onig_error_code_to_str(reinterpret_cast<OnigUChar*>(raw.data()), r, &einfo);
        d->error = QString(raw);
    }
}

Regex::~Regex()
{
}

bool Regex::isValid() const
{
    return d_func()->rx != 0;
}

QString Regex::error() const
{
    return d_func()->error;
}

QString Regex::pattern() const
{
    return d_func()->pattern;
}

void Regex::setPattern(const QString& pattern)
{
    Regex that(pattern);
    *this = that;
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
    int r = onig_search(d_func()->rx, uc(begin), uc(end), uc(offset), uc(range), match.d_func()->region, ONIG_OPTION_NONE);
    return (r != ONIG_MISMATCH);
}
