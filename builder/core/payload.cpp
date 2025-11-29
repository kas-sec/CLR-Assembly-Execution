#include "payload.h"
#include "../res/resource.h"
#include "../../common/crypto.h"
#include "../../common/overlay.h"

#include <windows.h>
#include <fstream>
#include <filesystem>
#include <vector>

namespace clr {
namespace builder {
namespace core {

namespace {

std::vector<uint8_t> LoadResource(int id) {
    HMODULE module = GetModuleHandle(nullptr);
    HRSRC res = FindResource(module, MAKEINTRESOURCE(id), RT_RCDATA);
    if (!res) return {};
    
    HGLOBAL loaded = ::LoadResource(module, res);
    if (!loaded) return {};
    
    DWORD size = SizeofResource(module, res);
    void* data = LockResource(loaded);
    if (!data) return {};
    
    std::vector<uint8_t> result(size);
    memcpy(result.data(), data, size);
    return result;
}

std::vector<uint8_t> ReadFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) return {};
    
    size_t size = static_cast<size_t>(file.tellg());
    file.seekg(0);
    
    std::vector<uint8_t> data(size);
    file.read(reinterpret_cast<char*>(data.data()), size);
    return data;
}

bool WriteFile(const std::string& path, const std::vector<uint8_t>& data) {
    std::ofstream file(path, std::ios::binary);
    if (!file) return false;
    
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    return file.good();
}

} // anonymous namespace

BuildResult Build(const std::string& input_path, const std::string& output_name, bool debug) {
    BuildResult result;
    
    int stub_id = debug ? IDR_LOADER_DEBUG : IDR_LOADER_RELEASE;
    auto stub = LoadResource(stub_id);
    if (stub.empty()) {
        result.error = "Failed to load embedded loader stub";
        return result;
    }
    
    auto assembly = ReadFile(input_path);
    if (assembly.empty()) {
        result.error = "Failed to read input file: " + input_path;
        return result;
    }
    result.original_size = assembly.size();
    
    uint8_t key[kKeySize];
    uint8_t iv[kIVSize];
    if (!crypto::RandomBytes(key, kKeySize) ||
        !crypto::RandomBytes(iv, kIVSize)) {
        result.error = "Failed to generate encryption keys";
        return result;
    }
    
    std::vector<uint8_t> encrypted;
    if (!crypto::Encrypt(assembly, encrypted, key, iv)) {
        result.error = "Failed to encrypt assembly";
        return result;
    }
    result.encrypted_size = encrypted.size();
    
    std::vector<uint8_t> payload;
    payload.reserve(stub.size() + encrypted.size() + sizeof(OverlayHeader));
    
    payload.insert(payload.end(), stub.begin(), stub.end());
    payload.insert(payload.end(), encrypted.begin(), encrypted.end());
    
    OverlayHeader header;
    header.magic = kOverlayMagic;
    header.encrypted_size = static_cast<uint32_t>(encrypted.size());
    memcpy(header.key, key, kKeySize);
    memcpy(header.iv, iv, kIVSize);
    
    auto header_ptr = reinterpret_cast<uint8_t*>(&header);
    payload.insert(payload.end(), header_ptr, header_ptr + sizeof(header));
    
    result.total_size = payload.size();
    
    std::filesystem::create_directories("outputs");
    
    result.output_path = "outputs/" + output_name + ".exe";
    if (!WriteFile(result.output_path, payload)) {
        result.error = "Failed to write output file: " + result.output_path;
        return result;
    }
    
    SecureZeroMemory(key, kKeySize);
    SecureZeroMemory(iv, kIVSize);
    SecureZeroMemory(assembly.data(), assembly.size());
    
    result.success = true;
    return result;
}

} // namespace core
} // namespace builder
} // namespace clr
