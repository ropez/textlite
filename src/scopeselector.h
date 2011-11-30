#ifndef SCOPESELECTOR_H
#define SCOPESELECTOR_H

#include <QtCore/QStack>
#include <QtCore/QStringList>


class ScopeSelector : public QStack<QStringList>
{
public:
    ScopeSelector(const QStack<QString>& other);

    ScopeSelector(const QString& selector);

    bool matches(const ScopeSelector& scope);
};

bool operator<(const ScopeSelector& lhs, const ScopeSelector& rhs);
bool listComparePrefix(const QStringList& list, const QStringList& prefix);

#endif // SCOPESELECTOR_H
