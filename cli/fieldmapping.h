#ifndef CLI_FIELDMAPPING
#define CLI_FIELDMAPPING

#include <array>
#include <cstddef>

namespace TagParser {
enum class KnownField : unsigned int;
}

namespace Cli {
namespace FieldMapping {

struct Mapping {
    const char *knownDenotation;
    TagParser::KnownField knownField;
};

const char *fieldDenotation(TagParser::KnownField knownField);
TagParser::KnownField knownField(const char *fieldDenotation, std::size_t fieldDenotationSize);
const std::array<Mapping, 99> &mapping();

} // namespace FieldMapping
} // namespace Cli

#endif // CLI_FIELDMAPPING
