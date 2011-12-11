#include "theme.h"
#include "scopeselector.h"

#include <QtGui/QTextCharFormat>

#include <QtDebug>

class ThemePrivate
{
    friend class Theme;

    QMap<ScopeSelector, QTextCharFormat> data;

    QColor parseThemeColor(const QString& hex);
};

Theme::Theme() :
    d(new ThemePrivate)
{
}

Theme::~Theme()
{
}

void Theme::clearThemeData()
{
    Theme that;
    d = that.d;
}

void Theme::setThemeData(const QVariantMap& themeData)
{
    clearThemeData();
    QVariantList settingsListData = themeData.value("settings").toList();
    QListIterator<QVariant> iter(settingsListData);
    while (iter.hasNext()) {
        QVariantMap itemData = iter.next().toMap();
        QTextCharFormat format;
        QVariantMap settingsData = itemData.value("settings").toMap();
        QMapIterator<QString, QVariant> settingsIter(settingsData);
        while (settingsIter.hasNext()) {
            settingsIter.next();
            QString key = settingsIter.key();
            if (key == "foreground") {
                QString hex = settingsIter.value().toString();
                format.setForeground(d->parseThemeColor(hex));
            } else if (key == "background") {
                QString hex = settingsIter.value().toString();
                format.setBackground(d->parseThemeColor(hex));
            } else if (key == "fontStyle") {
                QString styles = settingsIter.value().toString();
                QStringList list = styles.split(" ", QString::SkipEmptyParts);
                foreach (const QString& style, list) {
                    if (style == "bold") {
                        format.setFontWeight(75);
                    } else if (style == "italic") {
                        format.setFontItalic(true);
                    } else if (style == "underline") {
                        format.setFontUnderline(true);
                    } else {
                        qDebug() << "Unknown font style:" << style;
                    }
                }
            } else if (key == "caret") {
                QString color = settingsIter.value().toString();
                format.setProperty(QTextFormat::UserProperty, QBrush(d->parseThemeColor(color)));
            } else {
                qDebug() << "Unknown key in theme:" << key << "=>" << settingsIter.value();
            }
        }
        // XXX What to do when scope is not given?
        if (itemData.contains("name") && !itemData.contains("scope"))
            continue;
        QString scopes = itemData.value("scope").toString();
        foreach (QString scope, scopes.split(",")) {
            d->data[scope.trimmed()] = format;
        }
    }
}

QTextCharFormat Theme::format(const QString& name) const
{
    return d->data.value(name);
}

QTextCharFormat Theme::findFormat(const ScopeSelector& scope) const
{
    QTextCharFormat format;
    QMap<ScopeSelector, QTextCharFormat>::const_iterator it;
    for (it = d->data.begin(); it != d->data.end(); ++it) {
        ScopeSelector selector = it.key();
        if (selector.matches(scope)) {
            QTextCharFormat old = format;
            format = it.value();
            format.merge(old);
        }
    }
    return format;
}

QColor ThemePrivate::parseThemeColor(const QString& hex)
{
    QRegExp exp("#([\\w\\d]{6})([\\w\\d]{2})");
    if (exp.exactMatch(hex)) {
        bool ok = false;
        QString rgbahex = exp.cap(2) + exp.cap(1);
        QRgb rgba = rgbahex.toUInt(&ok, 16);
        if (ok) {
            return QColor::fromRgba(rgba);
        }
    }
    return QColor(hex);
}

bool operator==(const Theme& theme1, const Theme& theme2)
{
    return theme1.d == theme2.d;
}

bool operator!=(const Theme& theme1, const Theme& theme2)
{
    return theme1.d != theme2.d;
}
