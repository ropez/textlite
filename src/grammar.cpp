#include "grammar.h"

#include <QtDebug>

class GrammarPrivate
{
    friend class Grammar;

    RulePtr root;
    QMap<QString, RulePtr> grammars;
    QList<QMap<QString, RulePtr> > repositories;

    QMap<int, RulePtr> makeCaptures(const QVariantMap& capturesData);
    RulePtr makeRule(const QVariantMap& ruleData);
    QList<RulePtr> makeRuleList(const QVariantList& ruleListData);
};

Grammar::Grammar()
    : d(new GrammarPrivate)
{
}

Grammar::~Grammar()
{
}

RulePtr Grammar::root() const
{
    return d->root;
}

void Grammar::compile(const QMap<QString, QVariantMap>& syntaxData, const QString& scopeName)
{
    d->grammars.clear();
    d->repositories.clear();
    d->root = readSyntaxData(syntaxData[scopeName]);
    d->grammars[scopeName] = d->root;
    QMap<QString, RulePtr> repository = readRepository(syntaxData[scopeName]);
    resolveChildRules(syntaxData, repository, d->root, d->root, d->root);
}

RulePtr Grammar::readSyntaxData(const QVariantMap& syntaxData) const
{
    QVariantMap rootData;
    rootData["patterns"] = syntaxData.value("patterns");
    return d->makeRule(rootData);
}

QMap<QString, RulePtr> Grammar::readRepository(const QVariantMap& syntaxData) const
{
    QMap<QString, RulePtr> repository;
    QVariantMap repositoryData = syntaxData.value("repository").toMap();
    QMapIterator<QString, QVariant> iter(repositoryData);
    while (iter.hasNext()) {
        iter.next();
        QVariantMap ruleData = iter.value().toMap();
        repository["#" + iter.key()] = d->makeRule(ruleData);
    }
    d->repositories << repository;
    return repository;
}

QMap<int, RulePtr> GrammarPrivate::makeCaptures(const QVariantMap& capturesData)
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

RulePtr GrammarPrivate::makeRule(const QVariantMap& ruleData)
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

QList<RulePtr> GrammarPrivate::makeRuleList(const QVariantList& ruleListData)
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
                                RulePtr baseRule, RulePtr selfRule, RulePtr parentRule)
{
    if (parentRule->visited) return;
    parentRule->visited = true;

    QListIterator<RulePtr> iter(parentRule->patterns);
    while (iter.hasNext()) {
        RulePtr rule = iter.next();
        if (rule->includeName != QString()) {
            if (rule->includeName == "$base") {
                rule->include = baseRule;
            } else if (rule->includeName == "$self") {
                rule->include = selfRule;
            } else if (repository.contains(rule->includeName)) {
                rule->include = repository.value(rule->includeName);
                resolveChildRules(syntaxData, repository, baseRule, selfRule, rule->include);
            } else if (d->grammars.contains(rule->includeName)) {
                rule->include = d->grammars.value(rule->includeName);
            } else if (syntaxData.contains(rule->includeName)) {
                QVariantMap data = syntaxData[rule->includeName];
                RulePtr iRule = readSyntaxData(data);
                d->grammars[rule->includeName] = iRule;
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
