#include "./scriptapi.h"
#include "./fieldmapping.h"
#include "./helper.h"

#include "../application/knownfieldmodel.h"
#include "../dbquery/dbquery.h"
#include "../misc/utility.h"

#include <tagparser/abstracttrack.h>
#include <tagparser/exceptions.h>
#include <tagparser/id3/id3v2tag.h>
#include <tagparser/mediafileinfo.h>
#include <tagparser/progressfeedback.h>
#include <tagparser/signature.h>
#include <tagparser/tag.h>
#include <tagparser/tagvalue.h>
#include <tagparser/vorbis/vorbiscomment.h>

#include <qtutilities/misc/conversion.h>

#include <c++utilities/conversion/binaryconversion.h>
#include <c++utilities/conversion/conversionexception.h>
#include <c++utilities/conversion/stringbuilder.h>
#include <c++utilities/io/path.h>
#include <c++utilities/tests/testutils.h>

#include <qtutilities/misc/compat.h>

#include <QBuffer>
#include <QByteArray>
#include <QCoreApplication>
#include <QDir>
#include <QHash>
#include <QImage>
#include <QJSEngine>
#include <QJSValueIterator>
#include <QRegularExpression>

#include <filesystem>
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

const std::string UtilityObject::s_defaultContext = std::string("executing JavaScript");

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
    m_diag->emplace_back(mapping.value(level.toLower(), TagParser::DiagLevel::Debug), message.toStdString(),
        context.isEmpty() ? (m_context ? *m_context : s_defaultContext) : context.toStdString());
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
    return dir.exists() ? m_engine->toScriptValue(dir.entryList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot, QDir::Name)) : QJSValue();
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

QJSValue UtilityObject::openFile(const QString &path)
{
    if (!m_diag) {
        return QJSValue();
    }
    auto mediaFileInfo = std::make_unique<TagParser::MediaFileInfo>(path.toStdString());
    auto feedback = TagParser::AbortableProgressFeedback();
    try {
        mediaFileInfo->open(true);
        mediaFileInfo->parseEverything(*m_diag, feedback);
    } catch (const std::exception &e) {
        m_diag->emplace_back(TagParser::DiagLevel::Critical, CppUtilities::argsToString("Unable to open \"", mediaFileInfo->path(), "\": ", e.what()),
            m_context ? *m_context : s_defaultContext);
        return QJSValue();
    }
    auto mediaFileInfoObj = new MediaFileInfoObject(std::move(mediaFileInfo), *m_diag, m_engine, false, m_engine);
    return m_engine->newQObject(mediaFileInfoObj);
}

QJSValue UtilityObject::runProcess(const QString &path, const QJSValue &args, int timeout)
{
    auto res = m_engine->newObject();
#ifdef CPP_UTILITIES_HAS_EXEC_APP
    auto pathUtf8 = path.toUtf8();
    auto argsUtf8 = QByteArrayList();
    auto argsUtf8Array = std::vector<const char *>();
    if (args.isArray()) {
        const auto size = args.property(QStringLiteral("length")).toUInt();
        argsUtf8.reserve(size);
        argsUtf8Array.reserve(static_cast<std::size_t>(size) + 2);
        for (auto i = quint32(); i != size; ++i) {
            argsUtf8.append(args.property(i).toString().toUtf8());
        }
    }
    argsUtf8Array.emplace_back(pathUtf8.data());
    for (const auto &argUtf8 : argsUtf8) {
        argsUtf8Array.emplace_back(argUtf8.data());
    }
    argsUtf8Array.emplace_back(nullptr);
    auto output = std::string(), errors = std::string();
    try {
        auto exitStatus = CppUtilities::execHelperAppInSearchPath(pathUtf8.data(), argsUtf8Array.data(), output, errors, false, timeout);
#ifndef CPP_UTILITIES_BOOST_PROCESS
        if (WIFEXITED(exitStatus)) {
            exitStatus = WEXITSTATUS(exitStatus);
            res.setProperty(QStringLiteral("status"), exitStatus);
        }
#else
        res.setProperty(QStringLiteral("status"), exitStatus);
#endif
    } catch (const std::runtime_error &e) {
        res.setProperty(QStringLiteral("error"), QString::fromUtf8(e.what()));
    }
    res.setProperty(QStringLiteral("stdout"), QString::fromStdString(output));
    res.setProperty(QStringLiteral("stderr"), QString::fromStdString(errors));
#endif
    return res;
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

QJSValue UtilityObject::queryTekstowo(const QJSValue &songDescription)
{
    return m_engine->newQObject(QtGui::queryTekstowo(makeSongDescription(songDescription)));
}

QByteArray UtilityObject::convertImage(const QByteArray &imageData, const QSize &maxSize, const QString &format)
{
    auto image = QImage::fromData(imageData);
    if (image.isNull()) {
        return imageData;
    }
    if (!maxSize.isNull() && (image.width() > maxSize.width() || image.height() > maxSize.height())) {
        image = image.scaled(maxSize.width(), maxSize.height(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        if (image.isNull()) {
            return imageData;
        }
    }
    auto newData = QByteArray();
    auto buffer = QBuffer(&newData);
    auto res = buffer.open(QIODevice::WriteOnly) && image.save(&buffer, format.isEmpty() ? "JPEG" : format.toUtf8().data());
    return res ? newData : imageData;
}

static QString propertyString(const QJSValue &obj, const QString &propertyName)
{
    const auto val = obj.property(propertyName);
    return val.isUndefined() || val.isNull() ? QString() : val.toString();
}

QtGui::SongDescription UtilityObject::makeSongDescription(const QJSValue &obj)
{
    auto desc = QtGui::SongDescription();
    desc.songId = propertyString(obj, QStringLiteral("songId"));
    desc.title = propertyString(obj, QStringLiteral("title"));
    desc.album = propertyString(obj, QStringLiteral("album"));
    desc.albumId = propertyString(obj, QStringLiteral("albumId"));
    desc.artist = propertyString(obj, QStringLiteral("artist"));
    desc.artistId = propertyString(obj, QStringLiteral("artistId"));
    desc.year = propertyString(obj, QStringLiteral("year"));
    return desc;
}

PositionInSetObject::PositionInSetObject(TagParser::PositionInSet value, TagValueObject *relatedValue, QJSEngine *engine, QObject *parent)
    : QObject(parent)
    , m_v(value)
    , m_relatedValue(relatedValue)
{
    Q_UNUSED(engine)
}

PositionInSetObject::PositionInSetObject(const PositionInSetObject &other)
    : QObject(other.parent())
    , m_v(other.m_v)
    , m_relatedValue(nullptr)
{
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
    if (m_relatedValue) {
        m_relatedValue->flagChange();
    }
    m_v.setPosition(position);
}

qint32 PositionInSetObject::total() const
{
    return m_v.total();
}

void PositionInSetObject::setTotal(qint32 total)
{
    if (m_relatedValue) {
        m_relatedValue->flagChange();
    }
    m_v.setTotal(total);
}

QString PositionInSetObject::toString() const
{
    return QString::fromStdString(m_v.toString());
}

TagValueObject::TagValueObject(const TagParser::TagValue &value, QJSEngine *engine, QObject *parent)
    : QObject(parent)
    , m_engine(engine)
    , m_initial(true)
{
    setContentFromTagValue(value);
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
    flagChange();
    m_content = content;
}

void TagValueObject::setContentFromTagValue(const TagParser::TagValue &value)
{
    const auto type = TagParser::tagDataTypeString(value.type());
    m_type = QString::fromUtf8(type.data(), Utility::sizeToInt(type.size()));

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
        m_content = m_engine->toScriptValue(QByteArray(value.dataPointer(), Utility::sizeToInt(value.dataSize())));
        break;
    case TagParser::TagDataType::PositionInSet:
        m_content = m_engine->newQObject(new PositionInSetObject(value.toPositionInSet(), this, m_engine, this));
        break;
    default:
        m_content = QJSValue::NullValue;
    }
}

bool TagValueObject::isInitial() const
{
    return m_initial;
}

TagParser::TagValue TagValueObject::toTagValue(TagParser::TagTextEncoding encoding) const
{
    auto res = TagParser::TagValue();
    if (m_content.isUndefined() || m_content.isNull()) {
        return res;
    }
    if (const auto variant = m_content.toVariant(); variant.userType() == QMetaType::QByteArray) {
        const auto bytes = variant.toByteArray();
        const auto container = TagParser::parseSignature(bytes.data(), static_cast<std::size_t>(bytes.size()));
        res.assignData(bytes.data(), static_cast<std::size_t>(bytes.size()), TagParser::TagDataType::Binary);
        res.setMimeType(TagParser::containerMimeType(container));
    } else {
        const auto str = m_content.toString();
        res.assignText(reinterpret_cast<const char *>(str.utf16()), static_cast<std::size_t>(str.size()) * (sizeof(ushort) / sizeof(char)),
            nativeUtf16Encoding, encoding);
    }
    return res;
}

void TagValueObject::flagChange()
{
    if (m_initial) {
        if (const auto *const positionInSet = qobject_cast<const PositionInSetObject *>(m_content.toQObject())) {
            m_initialContent = m_engine->newQObject(new PositionInSetObject(*positionInSet));
        } else {
            m_initialContent = m_content;
        }
        m_initial = false;
    }
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

std::string TagObject::printJsValue(const QJSValue &value)
{
    const auto str = value.toString();
    for (const auto c : str) {
        if (!c.isPrint()) {
            return "[binary]";
        }
    }
    return str.toStdString();
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

/// \brief Sets the first of the specified \a values as front-cover with no description replacing any existing cover values.
template <class TagType> static void setId3v2CoverValues(TagType *tag, std::vector<TagParser::TagValue> &&values)
{
    auto &fields = tag->fields();
    const auto id = tag->fieldId(TagParser::KnownField::Cover);
    const auto range = fields.equal_range(id);
    const auto first = range.first;

    constexpr auto coverType = CoverType(3); // assume front cover
    constexpr auto description = std::optional<std::string_view>(); // assume no description

    for (auto &tagValue : values) {
        // check whether there is already a tag value with the current type and description
        auto pair = std::find_if(first, range.second, std::bind(&fieldPredicate<TagType>, coverType, description, std::placeholders::_1));
        if (pair != range.second) {
            // there is already a tag value with the current type and description
            // -> update this value
            pair->second.setValue(tagValue);
            // check whether there are more values with the current type and description
            while ((pair = std::find_if(++pair, range.second, std::bind(&fieldPredicate<TagType>, coverType, description, std::placeholders::_1)))
                != range.second) {
                // -> remove these values as we only support one value of a type/description in the same tag
                pair->second.setValue(TagParser::TagValue());
            }
        } else if (!tagValue.isEmpty()) {
            using FieldType = typename TagType::FieldType;
            auto newField = FieldType(id, tagValue);
            newField.setTypeInfo(static_cast<typename FieldType::TypeInfoType>(coverType));
            fields.insert(std::pair(id, std::move(newField)));
        }
        break; // allow only setting one value for now
    }
}

void TagObject::applyChanges()
{
    auto context = !m_tag.target().isEmpty() || m_tag.type() == TagParser::TagType::MatroskaTag
        ? CppUtilities::argsToString(m_tag.typeName(), " targeting ", m_tag.targetString())
        : std::string(m_tag.typeName());
    m_diag.emplace_back(TagParser::DiagLevel::Information, "applying changes", std::move(context));
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
            auto &value = values.emplace_back(tagValueObj->toTagValue(encoding));
            m_diag.emplace_back(TagParser::DiagLevel::Information,
                value.isNull()
                    ? CppUtilities::argsToString(" - delete ", propertyName.toStdString(), '[', i, ']')
                    : (tagValueObj->initialContent().isUndefined()
                            ? CppUtilities::argsToString(
                                " - set ", propertyName.toStdString(), '[', i, "] to '", printJsValue(tagValueObj->content()), '\'')
                            : ((tagValueObj->content().equals(tagValueObj->initialContent()))
                                    ? CppUtilities::argsToString(" - set ", propertyName.toStdString(), '[', i, "] to '",
                                        printJsValue(tagValueObj->content()), "\' (no change)")
                                    : CppUtilities::argsToString(" - change ", propertyName.toStdString(), '[', i, "] from '",
                                        printJsValue(tagValueObj->initialContent()), "' to '", printJsValue(tagValueObj->content()), '\''))),
                std::string());
        }
        // assign cover values of ID3v2/VorbisComment tags as front-cover with no description
        if (field == TagParser::KnownField::Cover && !values.empty()) {
            switch (m_tag.type()) {
            case TagParser::TagType::Id3v2Tag:
                setId3v2CoverValues(static_cast<TagParser::Id3v2Tag *>(&m_tag), std::move(values));
                continue;
            case TagParser::TagType::VorbisComment:
                setId3v2CoverValues(static_cast<TagParser::VorbisComment *>(&m_tag), std::move(values));
                continue;
            default:;
            }
        }
        m_tag.setValues(field, values);
    }
}

MediaFileInfoObject::MediaFileInfoObject(
    TagParser::MediaFileInfo &mediaFileInfo, TagParser::Diagnostics &diag, QJSEngine *engine, bool quiet, QObject *parent)
    : QObject(parent)
    , m_f(mediaFileInfo)
    , m_diag(diag)
    , m_engine(engine)
    , m_quiet(quiet)
{
}

MediaFileInfoObject::MediaFileInfoObject(
    std::unique_ptr<TagParser::MediaFileInfo> &&mediaFileInfo, TagParser::Diagnostics &diag, QJSEngine *engine, bool quiet, QObject *parent)
    : MediaFileInfoObject(*mediaFileInfo.get(), diag, engine, quiet, parent)
{
    m_f_owned = std::move(mediaFileInfo);
}

MediaFileInfoObject::~MediaFileInfoObject()
{
}

QString MediaFileInfoObject::path() const
{
    return QString::fromStdString(m_f.path());
}

bool MediaFileInfoObject::isPathRelative() const
{
    return QFileInfo(path()).isRelative();
}

QString MediaFileInfoObject::name() const
{
    return QString::fromStdString(m_f.fileName());
}

QString MediaFileInfoObject::nameWithoutExtension() const
{
    return QString::fromStdString(m_f.fileName(true));
}

QString MediaFileInfoObject::extension() const
{
    return QString::fromStdString(m_f.extension());
}

QString MediaFileInfoObject::containingDirectory() const
{
    return QString::fromStdString(m_f.containingDirectory());
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

bool MediaFileInfoObject::rename(const QString &newPath)
{
    const auto from = m_f.path();
    const auto fromNative = std::filesystem::path(CppUtilities::makeNativePath(from));
    const auto toRelUtf8 = newPath.toUtf8();
    const auto toRelView = CppUtilities::PathStringView(toRelUtf8.data(), static_cast<std::size_t>(toRelUtf8.size()));
    const auto toNative = fromNative.parent_path().append(CppUtilities::makeNativePath(toRelView));
    const auto toView = CppUtilities::extractNativePath(toNative.native());
    try {
        m_f.stream().close();
        std::filesystem::rename(fromNative, toNative);
        m_f.reportPathChanged(toView);
        m_f.stream().open(m_f.path(), std::ios_base::in | std::ios_base::out | std::ios_base::binary);
    } catch (const std::runtime_error &e) {
        m_diag.emplace_back(TagParser::DiagLevel::Critical, e.what(), CppUtilities::argsToString("renaming \"", from, "\" to \"", toView));
        return false;
    }
    if (!m_quiet) {
        std::cout << " - Renamed \"" << from << "\" to \"" << toView << "\"\n";
    }
    return true;
}

} // namespace Cli
