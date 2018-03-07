#ifndef CLI_FIELDMAPPING
#define CLI_FIELDMAPPING

#include <cstddef>

namespace TagParser {
enum class KnownField : unsigned int;
}

namespace Cli {
namespace FieldMapping {

const char *fieldDenotation(TagParser::KnownField knownField);
TagParser::KnownField knownField(const char *fieldDenotation, std::size_t fieldDenotationSize);

} // namespace FieldMapping
} // namespace Cli

#endif // CLI_FIELDMAPPING
