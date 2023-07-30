#ifndef CLI_MEDIA_FILE_INFO_OBJECT_H
#define CLI_MEDIA_FILE_INFO_OBJECT_H

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
    void exit(int retcode);
    QString readEnvironmentVariable(const QString &variable, const QString &defaultValue = QString()) const;
    QString formatName(const QString &str) const;
    QString fixUmlauts(const QString &str) const;

private:
    const std::string *m_context;
    TagParser::Diagnostics *m_diag;
};

inline void UtilityObject::setDiag(const std::string *context, TagParser::Diagnostics *diag)
{
    m_context = context;
    m_diag = diag;
}

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
    const QJSValue &initialContent() const;
    bool isInitial() const;
    TagParser::TagValue toTagValue(TagParser::TagTextEncoding encoding) const;

public Q_SLOTS:
    void restore();
    void clear();

private:
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
    Q_PROPERTY(QJSValue fields READ fields)

public:
    explicit TagObject(TagParser::Tag &tag, TagParser::Diagnostics &diag, QJSEngine *engine, QObject *parent);
    ~TagObject() override;

    TagParser::Tag &tag();
    QString type() const;
    QJSValue &fields();

public Q_SLOTS:
    void applyChanges();

private:
    static QString propertyNameForField(TagParser::KnownField field);

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
    Q_PROPERTY(QString currentPath READ currentPath)
    Q_PROPERTY(QString currentName READ currentName)
    Q_PROPERTY(QList<TagObject *> tags READ tags)

public:
    explicit MediaFileInfoObject(TagParser::MediaFileInfo &mediaFileInfo, TagParser::Diagnostics &diag, QJSEngine *engine, QObject *parent = nullptr);
    ~MediaFileInfoObject() override;

    TagParser::MediaFileInfo &fileInfo();
    QString currentPath() const;
    QString currentName() const;
    QList<TagObject *> &tags();

public Q_SLOTS:
    void applyChanges();

private:
    TagParser::MediaFileInfo &m_f;
    TagParser::Diagnostics &m_diag;
    QJSEngine *m_engine;
    QList<TagObject *> m_tags;
};

inline TagParser::MediaFileInfo &MediaFileInfoObject::fileInfo()
{
    return m_f;
}

} // namespace Cli

#endif // CLI_MEDIA_FILE_INFO_OBJECT_H
