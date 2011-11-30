#ifndef THEME_H
#define THEME_H

#include <QtCore/QSharedPointer>
#include <QtCore/QVariantMap>

class ScopeSelector;
class ThemePrivate;

class QTextCharFormat;

class Theme
{
public:
    Theme();
    ~Theme();

    void setThemeData(const QVariantMap& themeData);

    QTextCharFormat format(const QString& name) const;

    /**
      "string" => "string" -> ""
      "string.quoted" => "string.quited" -> "string" -> ""
      "string.koko","pun.ruby" => "string.koko pun.ruby", "string pun.ruby" -> "pun.ruby" -> "string pun" -> "pun" -> "string" -> ""
      "a", "b", "c" => "a b c" -> "b c" -> "a c" -> "c" -> "a b" -> "b" -> "a" -> ""
      */
    QTextCharFormat findFormat(const ScopeSelector& scope) const;

private:
    QSharedPointer<ThemePrivate> d;
};

#endif // THEME_H
