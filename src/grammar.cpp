#include "grammar.h"

#include <QtDebug>

class GrammarPrivate
{
    friend class Grammar;
};

Grammar::Grammar()
    : d(new GrammarPrivate)
{
}

Grammar::~Grammar()
{
}

RulePtr Grammar::compile(const QMap<QString, QVariantMap>& syntaxData, const QString& scopeName)
{
    RulePtr root = readSyntaxData(syntaxData[scopeName]);
    QMap<QString, RulePtr> repository = readRepository(syntaxData[scopeName]);
    resolveChildRules(syntaxData, repository, root, root, root);
    return root;
}

RulePtr Grammar::readSyntaxData(const QVariantMap& syntaxData) const
{
    QVariantMap rootData;
    rootData["patterns"] = syntaxData.value("patterns");
    return makeRule(rootData);
}

QMap<QString, RulePtr> Grammar::readRepository(const QVariantMap& syntaxData) const
{
    QMap<QString, RulePtr> repository;
    QVariantMap repositoryData = syntaxData.value("repository").toMap();
    QMapIterator<QString, QVariant> iter(repositoryData);
    while (iter.hasNext()) {
        iter.next();
        QVariantMap ruleData = iter.value().toMap();
        repository["#" + iter.key()] = makeRule(ruleData);
    }
    return repository;
}

QMap<int, RulePtr> Grammar::makeCaptures(const QVariantMap& capturesData) const
{
    QMap<int, RulePtr> captures;
    QMapIterator<QString, QVariant> iter(capturesData);
    while (iter.hasNext()) {
        iter.next();
        int num = iter.key().toInt();
        QVariantMap data = iter.value().toMap();
        captures[num] = makeRule(data);
    }
    return captures;
}

RulePtr Grammar::makeRule(const QVariantMap& ruleData) const
{
    RulePtr rule(new RuleData);

    rule->name = ruleData.value("name").toString();
    if (ruleData.contains("contentName"))
        rule->contentName = ruleData.value("contentName").toString();
    else
        rule->contentName = rule->name;
    rule->includeName = ruleData.value("include").toString();
    if (ruleData.contains("begin")) {
        rule->beginPattern = ruleData.value("begin").toString();
        rule->begin.setPattern(rule->beginPattern);
    }
    if (ruleData.contains("end")) {
        rule->endPattern = ruleData.value("end").toString();
    }
    if (ruleData.contains("match")) {
        rule->matchPattern = ruleData.value("match").toString();
        rule->match.setPattern(rule->matchPattern);
    }
    QVariant ruleListData = ruleData.value("patterns");
    if (ruleListData.isValid()) {
        rule->patterns = makeRuleList(ruleListData.toList());
    }

    QVariant capturesData = ruleData.value("captures");
    rule->captures = makeCaptures(capturesData.toMap());
    QVariant beginCapturesData = ruleData.value("beginCaptures");
    if (beginCapturesData.isValid())
        rule->beginCaptures = makeCaptures(beginCapturesData.toMap());
    else
        rule->beginCaptures = rule->captures;
    QVariant endCapturesData = ruleData.value("endCaptures");
    if (endCapturesData.isValid())
        rule->endCaptures = makeCaptures(endCapturesData.toMap());
    else
        rule->endCaptures = rule->captures;

    return rule;
}

QList<RulePtr> Grammar::makeRuleList(const QVariantList& ruleListData) const
{
    QList<RulePtr> rules;
    QListIterator<QVariant> iter(ruleListData);
    while (iter.hasNext()) {
        QVariantMap ruleData = iter.next().toMap();
        rules << makeRule(ruleData);
    }
    return rules;
}

void Grammar::resolveChildRules(const QMap<QString, QVariantMap>& syntaxData,
                                const QMap<QString, RulePtr>& repository,
                                RulePtr baseRule, RulePtr selfRule, RulePtr parentRule) const
{
    QListIterator<RulePtr> iter(parentRule->patterns);
    while (iter.hasNext()) {
        RulePtr rule = iter.next();
        if (rule->includeName != QString()) {
            if (rule->includeName == "$base") {
                rule->include = baseRule;
            } else if (rule->includeName == "$self") {
                rule->include = selfRule;
            } else if (selfRule->referenced.contains(rule->includeName)) {
                rule->include = selfRule->referenced[rule->includeName];
            } else if (repository.contains(rule->includeName)) {
                RulePtr rRule = repository.value(rule->includeName);
                selfRule->referenced[rule->includeName] = rRule;
                resolveChildRules(syntaxData, repository, baseRule, selfRule, rRule);
                rule->include = rRule;
            } else if (baseRule->referenced.contains(rule->includeName)) {
                rule->include = baseRule->referenced[rule->includeName];
            } else if (syntaxData.contains(rule->includeName)) {
                QVariantMap data = syntaxData[rule->includeName];
                RulePtr iRule = readSyntaxData(data);
                baseRule->referenced[rule->includeName] = iRule;
                QMap<QString, RulePtr> iRepo = readRepository(data);
                resolveChildRules(syntaxData, iRepo, baseRule, iRule, iRule);
                rule->include = iRule;
            } else {
                qWarning() << "Pattern not in repository" << rule->includeName;
            }
        } else {
            resolveChildRules(syntaxData, repository, baseRule, selfRule, rule);
        }
    }
}
