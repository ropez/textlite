#ifndef PLISTREADER_H
#define PLISTREADER_H

#include <QObject>
#include <QIODevice>
#include <QVariant>

class QXmlStreamReader;

class PlistReader : public QObject
{
    Q_OBJECT

public:
    PlistReader(QObject* parent = 0);

    QVariant read(const QString& path);
    QVariant read(QIODevice& device);

private:
    QVariant readDocument(QXmlStreamReader& reader);
    QVariant readElement(QXmlStreamReader& reader);
    QVariant readElementString(QXmlStreamReader& reader);
    QVariant readElementInteger(QXmlStreamReader& reader);
    QVariant readElementArray(QXmlStreamReader& reader);
    QVariant readElementDict(QXmlStreamReader& reader);
    void readWhiteSpace(QXmlStreamReader& reader);
};

#endif // PLISTREADER_H
