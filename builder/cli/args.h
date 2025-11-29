#ifndef CLR_BUILDER_CLI_ARGS_H
#define CLR_BUILDER_CLI_ARGS_H

#include <string>

namespace clr {
namespace builder {
namespace cli {

struct Arguments {
    std::string input;
    std::string output;
    bool debug = false;
    bool help = false;
    bool valid = false;
};

Arguments Parse(int argc, char* argv[]);
void PrintUsage(const char* program);
void PrintBanner();

} // namespace cli
} // namespace builder
} // namespace clr

#endif // CLR_BUILDER_CLI_ARGS_H
