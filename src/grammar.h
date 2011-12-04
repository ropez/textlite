#ifndef GRAMMAR_H
#define GRAMMAR_H

#include "regex.h"

#include <QtCore/QString>
#include <QtCore/QMap>
#include <QtCore/QVariant>

class GrammarPrivate;

struct RuleData;
typedef QSharedPointer<RuleData> RulePtr;
typedef QWeakPointer<RuleData> WeakRulePtr;

struct RuleData {
    RuleData() {}

    QString name;
    QString contentName;
    QString includeName;
    QString beginPattern;
    QString endPattern;
    QString matchPattern;
    Regex begin;
    Regex match;
    QMap<int, RulePtr> captures;
    QMap<int, RulePtr> beginCaptures;
    QMap<int, RulePtr> endCaptures;
    QList<RulePtr> patterns;
    WeakRulePtr include;

    QMap<QString, RulePtr> referenced;
};

class Grammar
{
public:
    Grammar();
    ~Grammar();

    RulePtr compile(const QMap<QString, QVariantMap>& syntaxData, const QString& scopeName);

private:
    RulePtr readSyntaxData(const QVariantMap &syntaxData) const;
    QMap<QString, RulePtr> readRepository(const QVariantMap& syntaxData) const;

    void resolveChildRules(const QMap<QString, QVariantMap>& syntaxData,
                           const QMap<QString, RulePtr>& repository,
                           RulePtr baseRule, RulePtr selfRule, RulePtr parentRule);

    QSharedPointer<GrammarPrivate> d;
};

#endif // GRAMMAR_H
