#include "cli/args.h"
#include "core/payload.h"

#include <iostream>
#include <iomanip>

int main(int argc, char* argv[]) {
    using namespace clr::builder;
    
    cli::PrintBanner();
    
    auto args = cli::Parse(argc, argv);
    
    if (args.help) {
        cli::PrintUsage(argv[0]);
        return 0;
    }
    
    if (!args.valid) {
        std::cerr << "[!] error: missing required arguments\n\n";
        cli::PrintUsage(argv[0]);
        return 1;
    }
    
    std::cout << "[*] input:  " << args.input << "\n";
    std::cout << "[*] output: " << args.output << "\n";
    std::cout << "[*] mode:   " << (args.debug ? "debug" : "release") << "\n\n";
    std::cout << "[*] building payload\n";
    
    auto result = core::Build(args.input, args.output, args.debug);
    
    if (!result.success) {
        std::cerr << "\n[!] build failed: " << result.error << "\n";
        return 1;
    }
    
    std::cout << "\n[+] build successful\n\n";
    std::cout << "    original:  " << std::setw(10) << result.original_size  << " bytes\n";
    std::cout << "    encrypted: " << std::setw(10) << result.encrypted_size << " bytes\n";
    std::cout << "    total:     " << std::setw(10) << result.total_size     << " bytes\n\n";
    std::cout << "[+] output: " << result.output_path << "\n\n";
    
    return 0;
}
