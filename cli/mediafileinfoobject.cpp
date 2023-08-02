#include "./mediafileinfoobject.h"
#include "./fieldmapping.h"

#include "../application/knownfieldmodel.h"
#include "../dbquery/dbquery.h"
#include "../misc/utility.h"

#include <tagparser/abstracttrack.h>
#include <tagparser/exceptions.h>
#include <tagparser/mediafileinfo.h>
#include <tagparser/progressfeedback.h>
#include <tagparser/tag.h>
#include <tagparser/tagvalue.h>

#include <qtutilities/misc/conversion.h>

#include <c++utilities/conversion/binaryconversion.h>
#include <c++utilities/conversion/conversionexception.h>

#include <qtutilities/misc/compat.h>

#include <QCoreApplication>
#include <QDir>
#include <QHash>
#include <QJSEngine>
#include <QJSValueIterator>
#include <QRegularExpression>

#include <iostream>
#include <limits>
#include <type_traits>

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
QT_BEGIN_NAMESPACE
uint qHash(const TagParser::KnownField key, uint seed) noexcept
{
    return ::qHash(static_cast<std::underlying_type_t<TagParser::KnownField>>(key), seed);
}
QT_END_NAMESPACE
#endif

namespace Cli {

constexpr auto nativeUtf16Encoding = TagParser::TagTextEncoding::
#if defined(CONVERSION_UTILITIES_BYTE_ORDER_LITTLE_ENDIAN)
    Utf16LittleEndian
#else
    Utf16BigEndian
#endif
    ;

UtilityObject::UtilityObject(QJSEngine *engine)
    : QObject(engine)
    , m_engine(engine)
    , m_context(nullptr)
    , m_diag(nullptr)
{
}

void UtilityObject::log(const QString &message)
{
    std::cout << message.toStdString() << std::endl;
}

void UtilityObject::diag(const QString &level, const QString &message, const QString &context)
{
    if (!m_diag) {
        return;
    }
    static const auto mapping = QHash<QString, TagParser::DiagLevel>({
        { QStringLiteral("fatal"), TagParser::DiagLevel::Fatal },
        { QStringLiteral("critical"), TagParser::DiagLevel::Critical },
        { QStringLiteral("error"), TagParser::DiagLevel::Critical },
        { QStringLiteral("warning"), TagParser::DiagLevel::Warning },
        { QStringLiteral("info"), TagParser::DiagLevel::Information },
        { QStringLiteral("information"), TagParser::DiagLevel::Information },
        { QStringLiteral("debug"), TagParser::DiagLevel::Debug },
    });
    static const auto defaultContext = std::string("executing JavaScript");
    m_diag->emplace_back(mapping.value(level.toLower(), TagParser::DiagLevel::Debug), message.toStdString(),
        context.isEmpty() ? (m_context ? *m_context : defaultContext) : context.toStdString());
}

int UtilityObject::exec()
{
    return QCoreApplication::exec();
}

void UtilityObject::exit(int retcode)
{
    QCoreApplication::exit(retcode);
}

QJSValue UtilityObject::readEnvironmentVariable(const QString &variable, const QJSValue &defaultValue) const
{
    const auto variableUtf8 = variable.toUtf8();
    if (qEnvironmentVariableIsSet(variableUtf8.data())) {
        return QJSValue(qEnvironmentVariable(variableUtf8.data()));
    } else {
        return QJSValue(defaultValue);
    }
}

QJSValue UtilityObject::readDirectory(const QString &path)
{
    auto dir = QDir(path);
    return dir.exists() ? m_engine->toScriptValue(dir.entryList(QDir::NoDotAndDotDot, QDir::Name)) : QJSValue();
}

QJSValue UtilityObject::readFile(const QString &path)
{
    if (auto file = QFile(path); file.open(QFile::ReadOnly)) {
        if (auto data = file.readAll(); file.error() != QFile::NoError) {
            return m_engine->toScriptValue(std::move(data));
        }
    }
    return QJSValue();
}

QString UtilityObject::formatName(const QString &str) const
{
    return Utility::formatName(str);
}

QString UtilityObject::fixUmlauts(const QString &str) const
{
    return Utility::fixUmlauts(str);
}

QJSValue UtilityObject::queryMusicBrainz(const QJSValue &songDescription)
{
    return m_engine->newQObject(QtGui::queryMusicBrainz(makeSongDescription(songDescription)));
}

QJSValue UtilityObject::queryLyricsWikia(const QJSValue &songDescription)
{
    return m_engine->newQObject(QtGui::queryLyricsWikia(makeSongDescription(songDescription)));
}

QJSValue UtilityObject::queryMakeItPersonal(const QJSValue &songDescription)
{
    return m_engine->newQObject(QtGui::queryMakeItPersonal(makeSongDescription(songDescription)));
}

QtGui::SongDescription UtilityObject::makeSongDescription(const QJSValue &obj)
{
    auto desc = QtGui::SongDescription(obj.property(QStringLiteral("songId")).toString());
    desc.title = obj.property(QStringLiteral("title")).toString();
    desc.album = obj.property(QStringLiteral("album")).toString();
    desc.albumId = obj.property(QStringLiteral("albumId")).toString();
    desc.artist = obj.property(QStringLiteral("artist")).toString();
    desc.artistId = obj.property(QStringLiteral("artistId")).toString();
    desc.year = obj.property(QStringLiteral("year")).toString();
    return desc;
}

PositionInSetObject::PositionInSetObject(TagParser::PositionInSet value, QJSEngine *engine, QObject *parent)
    : QObject(parent)
    , m_v(value)
{
    Q_UNUSED(engine)
}

PositionInSetObject::~PositionInSetObject()
{
}

qint32 PositionInSetObject::position() const
{
    return m_v.position();
}

void PositionInSetObject::setPosition(qint32 position)
{
    m_v.setPosition(position);
}

qint32 PositionInSetObject::total() const
{
    return m_v.total();
}

void PositionInSetObject::setTotal(qint32 total)
{
    return m_v.setTotal(total);
}

QString PositionInSetObject::toString() const
{
    return QString::fromStdString(m_v.toString());
}

TagValueObject::TagValueObject(const TagParser::TagValue &value, QJSEngine *engine, QObject *parent)
    : QObject(parent)
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    , m_type(QString::fromUtf8(TagParser::tagDataTypeString(value.type())))
#endif
    , m_initial(true)
{
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    const auto type = TagParser::tagDataTypeString(value.type());
    m_type = QString::fromUtf8(type.data(), Utility::sizeToInt(type.size()));
#endif

    switch (value.type()) {
    case TagParser::TagDataType::Undefined:
        break;
    case TagParser::TagDataType::Text:
    case TagParser::TagDataType::Popularity:
    case TagParser::TagDataType::DateTime:
    case TagParser::TagDataType::DateTimeExpression:
    case TagParser::TagDataType::TimeSpan:
        m_content = Utility::tagValueToQString(value);
        break;
    case TagParser::TagDataType::Integer:
        m_content = value.toInteger();
        break;
    case TagParser::TagDataType::UnsignedInteger:
        if (auto v = value.toUnsignedInteger(); v < std::numeric_limits<uint>::max()) {
            m_content = QJSValue(static_cast<uint>(v));
        } else {
            m_content = QString::number(v);
        }
        break;
    case TagParser::TagDataType::Binary:
    case TagParser::TagDataType::Picture:
        m_content = engine->toScriptValue(QByteArray(value.dataPointer(), Utility::sizeToInt(value.dataSize())));
        break;
    case TagParser::TagDataType::PositionInSet:
        m_content = engine->newQObject(new PositionInSetObject(value.toPositionInSet(), engine, this));
        break;
    default:
        m_content = QJSValue::NullValue;
    }
}

TagValueObject::~TagValueObject()
{
}

const QString &TagValueObject::type() const
{
    return m_type;
}

const QJSValue &TagValueObject::content() const
{
    return m_content;
}

const QJSValue &TagValueObject::initialContent() const
{
    return m_initial ? m_content : m_initialContent;
}

void TagValueObject::setContent(const QJSValue &content)
{
    if (m_initial) {
        m_initialContent = m_content;
        m_initial = false;
    }
    m_content = content;
}

bool TagValueObject::isInitial() const
{
    return m_initial;
}

TagParser::TagValue TagValueObject::toTagValue(TagParser::TagTextEncoding encoding) const
{
    if (m_content.isUndefined() || m_content.isNull()) {
        return TagParser::TagValue();
    }
    const auto str = m_content.toString();
    return TagParser::TagValue(reinterpret_cast<const char *>(str.utf16()), static_cast<std::size_t>(str.size()) * (sizeof(ushort) / sizeof(char)),
        nativeUtf16Encoding, encoding);
}

void TagValueObject::restore()
{
    if (!m_initial) {
        m_content = m_initialContent;
        m_initial = true;
    }
}

void TagValueObject::clear()
{
    setContent(QJSValue());
}

QString TagValueObject::toString() const
{
    return m_content.toString();
}

TagObject::TagObject(TagParser::Tag &tag, TagParser::Diagnostics &diag, QJSEngine *engine, QObject *parent)
    : QObject(parent)
    , m_tag(tag)
    , m_diag(diag)
    , m_engine(engine)
{
}

TagObject::~TagObject()
{
}

QString TagObject::type() const
{
    return Utility::qstr(m_tag.typeName());
}

QJSValue TagObject::target() const
{
    const auto &target = m_tag.target();
    auto obj = m_engine->newObject();
    obj.setProperty(QStringLiteral("level"), m_engine->toScriptValue(target.level()));
    obj.setProperty(QStringLiteral("levelName"), QJSValue(QString::fromStdString(target.levelName())));
    obj.setProperty(QStringLiteral("tracks"), m_engine->toScriptValue(QList<std::uint64_t>(target.tracks().cbegin(), target.tracks().cend())));
    obj.setProperty(QStringLiteral("chapters"), m_engine->toScriptValue(QList<std::uint64_t>(target.chapters().cbegin(), target.chapters().cend())));
    obj.setProperty(QStringLiteral("editions"), m_engine->toScriptValue(QList<std::uint64_t>(target.editions().cbegin(), target.editions().cend())));
    obj.setProperty(
        QStringLiteral("attachments"), m_engine->toScriptValue(QList<std::uint64_t>(target.attachments().cbegin(), target.attachments().cend())));
    return obj;
}

QString TagObject::propertyNameForField(TagParser::KnownField field)
{
    static const auto reverseMapping = [] {
        auto reverse = QHash<TagParser::KnownField, QString>();
        for (const auto &mapping : FieldMapping::mapping()) {
            auto d = QString::fromUtf8(mapping.knownDenotation);
            if (d.isUpper()) {
                d = d.toLower(); // turn abbreviations into just lower case
            } else {
                d.front() = d.front().toLower();
            }
            reverse[mapping.knownField] = std::move(d);
        }
        return reverse;
    }();
    return reverseMapping.value(field, QString());
}

QJSValue &TagObject::fields()
{
    if (!m_fields.isUndefined()) {
        return m_fields;
    }
    static const auto fieldRegex = QRegularExpression(QStringLiteral("\\s(\\w)"));
    m_fields = m_engine->newObject();
    for (auto field = TagParser::firstKnownField; field != TagParser::KnownField::Invalid; field = TagParser::nextKnownField(field)) {
        if (!m_tag.supportsField(field)) {
            continue;
        }
        if (const auto propertyName = propertyNameForField(field); !propertyName.isEmpty()) {
            const auto values = m_tag.values(field);
            const auto size = Utility::sizeToInt<quint32>(values.size());
            auto array = m_engine->newArray(size);
            for (auto i = quint32(); i != size; ++i) {
                array.setProperty(i, m_engine->newQObject(new TagValueObject(m_tag.value(field), m_engine, this)));
            }
            m_fields.setProperty(propertyName, array);
        }
    }
    return m_fields;
}

void TagObject::applyChanges()
{
    auto context = !m_tag.target().isEmpty() || m_tag.type() == TagParser::TagType::MatroskaTag
        ? CppUtilities::argsToString(m_tag.typeName(), " targeting ", m_tag.targetString())
        : std::string(m_tag.typeName());
    m_diag.emplace_back(TagParser::DiagLevel::Debug, "applying changes", std::move(context));
    if (m_fields.isUndefined()) {
        return;
    }
    const auto encoding = m_tag.proposedTextEncoding();
    for (auto field = TagParser::firstKnownField; field != TagParser::KnownField::Invalid; field = TagParser::nextKnownField(field)) {
        if (!m_tag.supportsField(field)) {
            continue;
        }
        const auto propertyName = propertyNameForField(field);
        if (propertyName.isEmpty()) {
            continue;
        }
        auto propertyValue = m_fields.property(propertyName);
        if (!propertyValue.isArray()) {
            auto array = m_engine->newArray(1);
            array.setProperty(0, propertyValue);
            propertyValue = std::move(array);
        }
        const auto size = propertyValue.property(QStringLiteral("length")).toUInt();
        const auto initialValues = m_tag.values(field);
        auto values = std::vector<TagParser::TagValue>();
        values.reserve(size);
        for (auto i = quint32(); i != size; ++i) {
            const auto arrayElement = propertyValue.property(i);
            if (arrayElement.isUndefined() || arrayElement.isNull()) {
                continue;
            }
            auto *tagValueObj = qobject_cast<TagValueObject *>(arrayElement.toQObject());
            if (!tagValueObj) {
                tagValueObj = new TagValueObject(i < initialValues.size() ? *initialValues[i] : TagParser::TagValue(), m_engine, this);
                tagValueObj->setContent(arrayElement);
                propertyValue.setProperty(i, m_engine->newQObject(tagValueObj));
            }
            if (tagValueObj->isInitial()) {
                if (i < initialValues.size()) {
                    values.emplace_back(*initialValues[i]);
                }
                continue;
            }
            const auto &value = values.emplace_back(tagValueObj->toTagValue(encoding));
            m_diag.emplace_back(TagParser::DiagLevel::Debug,
                value.isNull()
                    ? CppUtilities::argsToString(" - delete ", propertyName.toStdString(), '[', i, ']')
                    : (tagValueObj->initialContent().isUndefined() ? CppUtilities::argsToString(
                           " - set ", propertyName.toStdString(), '[', i, "] to '", tagValueObj->content().toString().toStdString(), '\'')
                                                                   : CppUtilities::argsToString(" - change ", propertyName.toStdString(), '[', i,
                                                                       "] from '", tagValueObj->initialContent().toString().toStdString(), "' to '",
                                                                       tagValueObj->content().toString().toStdString(), '\'')),
                std::string());
        }
        m_tag.setValues(field, values);
    }
}

MediaFileInfoObject::MediaFileInfoObject(TagParser::MediaFileInfo &mediaFileInfo, TagParser::Diagnostics &diag, QJSEngine *engine, QObject *parent)
    : QObject(parent)
    , m_f(mediaFileInfo)
    , m_diag(diag)
    , m_engine(engine)
{
}

MediaFileInfoObject::~MediaFileInfoObject()
{
}

QString MediaFileInfoObject::currentPath() const
{
    return QString::fromStdString(m_f.path());
}

QString MediaFileInfoObject::currentName() const
{
    return QString::fromStdString(m_f.fileName());
}

QString MediaFileInfoObject::savePath() const
{
    return QtUtilities::fromNativeFileName(m_f.saveFilePath());
}

void MediaFileInfoObject::setSavePath(const QString &path)
{
    const auto nativePath = QtUtilities::toNativeFileName(path);
    m_f.setSaveFilePath(std::string_view(nativePath.data(), static_cast<std::size_t>(nativePath.size())));
}

QList<Cli::TagObject *> &MediaFileInfoObject::tags()
{
    if (!m_tags.isEmpty()) {
        return m_tags;
    }
    auto tags = m_f.tags();
    m_tags.reserve(Utility::sizeToInt(tags.size()));
    for (auto *const tag : tags) {
        m_tags << new TagObject(*tag, m_diag, m_engine, this);
    }
    return m_tags;
}

void MediaFileInfoObject::applyChanges()
{
    for (auto *const tag : m_tags) {
        tag->applyChanges();
    }
}

} // namespace Cli
