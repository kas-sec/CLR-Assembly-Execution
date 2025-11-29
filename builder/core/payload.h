#ifndef CLR_BUILDER_CORE_PAYLOAD_H
#define CLR_BUILDER_CORE_PAYLOAD_H

#include <string>
#include <cstddef>

namespace clr {
namespace builder {
namespace core {

struct BuildResult {
    bool success = false;
    std::string error;
    std::string output_path;
    size_t original_size = 0;
    size_t encrypted_size = 0;
    size_t total_size = 0;
};

BuildResult Build(const std::string& input_path, const std::string& output_name, bool debug);

} // namespace core
} // namespace builder
} // namespace clr

#endif // CLR_BUILDER_CORE_PAYLOAD_H
