#ifndef CLI_SCRIPT_API_H
#define CLI_SCRIPT_API_H

#include <tagparser/positioninset.h>

#include <QtGlobal>
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
namespace TagParser {
enum class KnownField : unsigned int;
}
QT_BEGIN_NAMESPACE
uint qHash(const TagParser::KnownField key, uint seed = 0) noexcept;
QT_END_NAMESPACE
#endif

#include <QJSValue>
#include <QObject>

QT_FORWARD_DECLARE_CLASS(QJSEngine)

namespace TagParser {
class Diagnostics;
class MediaFileInfo;
class Tag;
class TagValue;
enum class KnownField : unsigned int;
enum class TagTextEncoding : unsigned int;
} // namespace TagParser

namespace QtGui {
struct SongDescription;
}

namespace Cli {

/*!
 * \brief The UtilityObject class wraps useful functions of Qt, TagParser and the Utility namespace for use within QML.
 */
class UtilityObject : public QObject {
    Q_OBJECT

public:
    explicit UtilityObject(QJSEngine *engine);

    void setDiag(const std::string *context, TagParser::Diagnostics *diag);

public Q_SLOTS:
    void log(const QString &message);
    void diag(const QString &level, const QString &message, const QString &context = QString());

    int exec();
    void exit(int retcode = 0);

    QJSValue readEnvironmentVariable(const QString &variable, const QJSValue &defaultValue = QJSValue()) const;
    QJSValue readDirectory(const QString &path);
    QJSValue readFile(const QString &path);

    QString formatName(const QString &str) const;
    QString fixUmlauts(const QString &str) const;

    QJSValue queryMusicBrainz(const QJSValue &songDescription);
    QJSValue queryLyricsWikia(const QJSValue &songDescription);
    QJSValue queryMakeItPersonal(const QJSValue &songDescription);
    QJSValue queryTekstowo(const QJSValue &songDescription);

    QByteArray convertImage(const QByteArray &imageData, const QSize &maxSize, const QString &format);

private:
    static QtGui::SongDescription makeSongDescription(const QJSValue &obj);

    QJSEngine *m_engine;
    const std::string *m_context;
    TagParser::Diagnostics *m_diag;
};

inline void UtilityObject::setDiag(const std::string *context, TagParser::Diagnostics *diag)
{
    m_context = context;
    m_diag = diag;
}

class TagValueObject;

/*!
 * \brief The PositionInSetObject class wraps a TagParser::PositionInSet for use within QML.
 */
class PositionInSetObject : public QObject {
    Q_OBJECT
    Q_PROPERTY(qint32 position READ position WRITE setPosition)
    Q_PROPERTY(qint32 total READ total WRITE setTotal)

public:
    explicit PositionInSetObject(TagParser::PositionInSet value, TagValueObject *relatedValue, QJSEngine *engine, QObject *parent);
    explicit PositionInSetObject(const PositionInSetObject &other);
    ~PositionInSetObject() override;

    qint32 position() const;
    void setPosition(qint32 position);
    qint32 total() const;
    void setTotal(qint32 total);

public Q_SLOTS:
    QString toString() const;

private:
    TagParser::PositionInSet m_v;
    TagValueObject *m_relatedValue;
};

/*!
 * \brief The TagValueObject class wraps a TagParser::TagValue for use within QML.
 */
class TagValueObject : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString type READ type)
    Q_PROPERTY(QJSValue content READ content WRITE setContent)
    Q_PROPERTY(QJSValue initialContent READ initialContent)
    Q_PROPERTY(bool initial READ isInitial)

public:
    explicit TagValueObject(const TagParser::TagValue &value, QJSEngine *engine, QObject *parent);
    ~TagValueObject() override;

    const QString &type() const;
    const QJSValue &content() const;
    void setContent(const QJSValue &content);
    void setContentFromTagValue(const TagParser::TagValue &value);
    const QJSValue &initialContent() const;
    bool isInitial() const;
    TagParser::TagValue toTagValue(TagParser::TagTextEncoding encoding) const;

public Q_SLOTS:
    void restore();
    void clear();
    QString toString() const;
    void flagChange();

private:
    QJSEngine *m_engine;
    QString m_type;
    QJSValue m_content;
    QJSValue m_initialContent;
    bool m_initial;
};

/*!
 * \brief The TagObject class wraps a TagParser::Tag for use within QML.
 */
class TagObject : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString type READ type)
    Q_PROPERTY(QJSValue target READ target)
    Q_PROPERTY(QJSValue fields READ fields)

public:
    explicit TagObject(TagParser::Tag &tag, TagParser::Diagnostics &diag, QJSEngine *engine, QObject *parent);
    ~TagObject() override;

    TagParser::Tag &tag();
    QString type() const;
    QJSValue target() const;
    QJSValue &fields();

public Q_SLOTS:
    void applyChanges();

private:
    static QString propertyNameForField(TagParser::KnownField field);
    std::string printJsValue(const QJSValue &value);

    TagParser::Tag &m_tag;
    TagParser::Diagnostics &m_diag;
    QJSEngine *m_engine;
    QString m_type;
    QJSValue m_fields;
};

inline TagParser::Tag &TagObject::tag()
{
    return m_tag;
}

/*!
 * \brief The MediaFileInfoObject class wraps a TagParser::MediaFileInfo for use within QML.
 */
class MediaFileInfoObject : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString path READ path)
    Q_PROPERTY(QString name READ name)
    Q_PROPERTY(QString extension READ extension)
    Q_PROPERTY(QString containingDirectory READ containingDirectory)
    Q_PROPERTY(QString savePath READ savePath WRITE setSavePath)
    Q_PROPERTY(QList<TagObject *> tags READ tags)

public:
    explicit MediaFileInfoObject(
        TagParser::MediaFileInfo &mediaFileInfo, TagParser::Diagnostics &diag, QJSEngine *engine, bool quiet, QObject *parent = nullptr);
    ~MediaFileInfoObject() override;

    TagParser::MediaFileInfo &fileInfo();
    QString path() const;
    QString name() const;
    QString extension() const;
    QString containingDirectory() const;
    QString savePath() const;
    void setSavePath(const QString &path);
    QList<TagObject *> &tags();

public Q_SLOTS:
    void applyChanges();
    bool rename(const QString &newPath);

private:
    TagParser::MediaFileInfo &m_f;
    TagParser::Diagnostics &m_diag;
    QJSEngine *m_engine;
    QList<TagObject *> m_tags;
    bool m_quiet;
};

inline TagParser::MediaFileInfo &MediaFileInfoObject::fileInfo()
{
    return m_f;
}

} // namespace Cli

#endif // CLI_SCRIPT_API_H
