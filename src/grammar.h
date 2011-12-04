#ifndef GRAMMAR_H
#define GRAMMAR_H

#include <QtCore/QString>
#include <QtCore/QMap>
#include <QtCore/QVariant>
#include <QtCore/QSharedPointer>
#include <QtCore/QWeakPointer>

class GrammarPrivate;

struct RuleData;
typedef QSharedPointer<RuleData> RulePtr;
typedef QWeakPointer<RuleData> WeakRulePtr;

class Grammar
{
public:
    Grammar();
    ~Grammar();

    RulePtr compile(const QMap<QString, QVariantMap>& syntaxData, const QString& scopeName);

private:
    RulePtr readSyntaxData(const QVariantMap &syntaxData) const;
    QMap<QString, RulePtr> readRepository(const QVariantMap& syntaxData) const;
    QMap<int, RulePtr> makeCaptures(const QVariantMap& capturesData) const;
    RulePtr makeRule(const QVariantMap& ruleData) const;
    QList<RulePtr> makeRuleList(const QVariantList& ruleListData) const;

    void resolveChildRules(const QMap<QString, QVariantMap>& syntaxData,
                           const QMap<QString, RulePtr>& repository,
                           RulePtr baseRule, RulePtr selfRule, RulePtr parentRule) const;

    QSharedPointer<GrammarPrivate> d;
};

#endif // GRAMMAR_H
