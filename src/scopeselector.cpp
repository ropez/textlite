#include "scopeselector.h"

ScopeSelector::ScopeSelector(const QStack<QString>& other)
{
    QVectorIterator<QString> it(other);
    while (it.hasNext()) {
        QString element = it.next();
        if (!element.isEmpty()) {
            push(element.split("."));
        }
    }
}

ScopeSelector::ScopeSelector(const QString& selector)
{
    if (selector.isEmpty())
        return;

    QStringListIterator it(selector.simplified().split(" "));
    while (it.hasNext()) {
        push(it.next().split("."));
    }
}

bool ScopeSelector::matches(const ScopeSelector& scope)
{
    int l = scope.size();
    for (int i = 0; i < size(); i++) {
        const QStringList& s = at(size() - 1 - i);
        while (true) {
            if (l == 0)
                return false;
            const QStringList& x = scope[--l];
            if (listComparePrefix(x, s))
                break;
        }
    }
    return true;
}

bool operator<(const ScopeSelector& lhs, const ScopeSelector& rhs)
{
    if (lhs == rhs)
        return false;

    int size = qMax(lhs.size(), rhs.size());
    for (int i = 0; i < size; i++) {
        if (i >= lhs.size())
            return false;
        if (i >= rhs.size())
            return true;
        QStringList l = lhs[lhs.size() - 1 - i];
        QStringList r = rhs[rhs.size() - 1 - i];
        if (l != r) {
            int sz = qMax(l.size(), r.size());
            for (int j = 0; j < sz; j++) {
                if (j >= l.size())
                    return false;
                if (j >= r.size())
                    return true;
                if (l[j] != r[j])
                    return l[j] < r[j];
            }
            Q_ASSERT(false);
        }
    }
    return false;
}

bool listComparePrefix(const QStringList& list, const QStringList& prefix)
{
    if (list.length() < prefix.length())
        return false;

    for (int i = 0; i < prefix.length(); i++) {
        if (list[i] != prefix[i])
            return false;
    }
    return true;
}
