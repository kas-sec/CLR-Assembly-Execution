#include "args.h"

#include <iostream>

namespace clr {
namespace builder {
namespace cli {

Arguments Parse(int argc, char* argv[]) {
    Arguments args;
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            args.help = true;
            args.valid = true;
            return args;
        }
        else if (arg == "-i" || arg == "--input") {
            if (i + 1 < argc) args.input = argv[++i];
        }
        else if (arg == "-o" || arg == "--output") {
            if (i + 1 < argc) args.output = argv[++i];
        }
        else if (arg == "-d" || arg == "--debug") {
            args.debug = true;
        }
    }
    
    args.valid = !args.input.empty() && !args.output.empty();
    return args;
}

void PrintUsage(const char* program) {
    std::cout << "Usage: " << program << " [options]\n\n"
              << "Options:\n"
              << "  -i, --input <file>   Input .NET assembly (required)\n"
              << "  -o, --output <name>  Output filename (required)\n"
              << "  -d, --debug          Build with debug console output\n"
              << "  -h, --help           Show this help message\n\n"
              << "Example:\n"
              << "  " << program << " --input payload.exe --output stealth\n"
              << "  " << program << " -i payload.exe -o test --debug\n\n"
              << "Output will be placed in the 'outputs' directory.\n";
}

void PrintBanner() {
    std::cout << "\n"
              << "  CLR Assembly Builder\n"
              << "  ====================\n\n";
}

} // namespace cli
} // namespace builder
} // namespace clr
