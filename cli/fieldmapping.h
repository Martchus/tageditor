#ifndef CLI_FIELDMAPPING
#define CLI_FIELDMAPPING

#include <cstddef>

namespace Media {
enum class KnownField : unsigned int;
}

namespace Cli {
namespace FieldMapping {

const char *fieldDenotation(Media::KnownField knownField);
Media::KnownField knownField(const char *fieldDenotation, std::size_t fieldDenotationSize);

}
}

#endif // CLI_FIELDMAPPING
