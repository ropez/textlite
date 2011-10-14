#include "plistreader.h"

#include <QtXml/QXmlStreamReader>
#include <QFile>

#include <QtDebug>

PlistReader::PlistReader(QObject* parent)
    : QObject(parent)
{
}

QVariant PlistReader::read(const QString &path)
{
    QFile file(path);
    if (file.open(QFile::ReadOnly)) {
        return read(file);
    } else {
        return QVariant();
    }
}

QVariant PlistReader::read(QIODevice &device)
{
    QXmlStreamReader reader(&device);
    return readDocument(reader);
}

QVariant PlistReader::readDocument(QXmlStreamReader& reader)
{
    QVariant value;
    while (!reader.atEnd()) {
        reader.readNext();
        switch (reader.tokenType()) {
        case QXmlStreamReader::StartDocument:
            break;
        case QXmlStreamReader::DTD:
            if (reader.dtdName() != "plist") {
                qWarning() << "Unexpected doctype:" << reader.dtdName();
            }
            break;
        case QXmlStreamReader::StartElement:
            if (reader.name() != "plist") {
                value = readElement(reader);
            }
            break;
        case QXmlStreamReader::EndElement:
            if (reader.name() != "plist") {
                qWarning() << "Unexpected end tag:" << reader.name();
            }
            break;
        case QXmlStreamReader::EndDocument:
            break;
        case QXmlStreamReader::Comment:
            break;
        default:
            readWhiteSpace(reader);
            break;
        }
    }
    if (reader.hasError()) {
        qWarning() << reader.errorString();
        return QVariant();
    }
    return value;
}

QVariant PlistReader::readElement(QXmlStreamReader &reader) {
    if (reader.name() == "string") {
        return readElementString(reader);
    } else if (reader.name() == "integer") {
        return readElementInteger(reader);
    } else if (reader.name() == "array") {
        return readElementArray(reader);
    } else if (reader.name() == "dict") {
        return readElementDict(reader);
    } else {
        qWarning() << "Don't know how to read element" << reader.name();
        reader.readElementText(QXmlStreamReader::IncludeChildElements);
        return QVariant();
    }
}

QVariant PlistReader::readElementString(QXmlStreamReader &reader)
{
    return reader.readElementText();
}

QVariant PlistReader::readElementInteger(QXmlStreamReader &reader)
{
    QString data = reader.readElementText();
    QTextStream ts(&data);
    int i;
    ts >> i;
    return QVariant(i);
}

QVariant PlistReader::readElementArray(QXmlStreamReader& reader)
{
    QVariantList array;
    while (!reader.atEnd()) {
        reader.readNext();
        switch (reader.tokenType()) {
        case QXmlStreamReader::StartElement:
            array.append(readElement(reader));
            break;
        case QXmlStreamReader::EndElement:
            if (reader.name() == "array") {
                return array;
            } else {
                qWarning() << "Unexpected end tag:" << reader.name();
            }
            break;
        default:
            readWhiteSpace(reader);
            break;
        }
    }
    return QVariant();
}

QVariant PlistReader::readElementDict(QXmlStreamReader& reader)
{
    QMap<QString, QVariant> map;
    QString pName;
    while (!reader.atEnd()) {
        reader.readNext();
        switch (reader.tokenType()) {
        case QXmlStreamReader::StartElement:
            if (reader.name() == "key") {
                pName = reader.readElementText();
            } else if (!pName.isNull()) {
                map.insert(pName, readElement(reader));
                pName.clear();
            } else {
                qWarning() << "Expected key, got" << reader.name();
            }
            break;
        case QXmlStreamReader::EndElement:
            if (reader.name() == "dict") {
                return map;
            } else {
                qWarning() << "Unexpected end tag:" << reader.name();
            }
        default:
            readWhiteSpace(reader);
            break;
        }
    }
    return QVariant();
}

void PlistReader::readWhiteSpace(QXmlStreamReader &reader)
{
    if (reader.tokenType() != QXmlStreamReader::Characters) {
        qWarning() << "Don't know what to do with" << reader.tokenString();
    } else if (!reader.isWhitespace()) {
        qWarning() << "Unexpected content:" << reader.text();
    }
}
