#ifndef RULEDATA_H
#define RULEDATA_H

#include "grammar.h"
#include "regex.h"

/** @internal */

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

#endif // RULEDATA_H
