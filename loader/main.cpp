#include "../common/crypto.h"
#include "../common/overlay.h"
#include "clr.h"

#include <windows.h>
#include <vector>

#ifdef DEBUG_BUILD
#include <iostream>
#include <iomanip>

#define DBG(fmt, ...) printf("[DEBUG] " fmt, ##__VA_ARGS__)
#define DBG_HR(hr) printf("  HRESULT: 0x%08lX\n", (unsigned long)(hr))
#define DBG_SIZE(name, size) printf("  %s: %zu bytes\n", name, (size_t)(size))

void InitConsole() {
    AllocConsole();
    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONOUT$", "w", stderr);
    
    printf("\n");
    printf("==========================================\n");
    printf("  CLR Assembly Loader - DEBUG BUILD\n");
    printf("==========================================\n\n");
}

void WaitForExit() {
    printf("\n[DEBUG] Press Enter to exit...\n");
    getchar();
}

#else
#define DBG(fmt, ...)
#define DBG_HR(hr)
#define DBG_SIZE(name, size)
#define InitConsole()
#define WaitForExit()
#endif

namespace {

bool ReadOverlay(std::vector<uint8_t>& data, clr::OverlayHeader& header) {
    DBG("Reading overlay from self...\n");
    
    wchar_t path[MAX_PATH];
    if (!GetModuleFileNameW(nullptr, path, MAX_PATH)) {
        DBG("  GetModuleFileNameW failed: %lu\n", GetLastError());
        return false;
    }
    
#ifdef DEBUG_BUILD
    wprintf(L"  Executable: %s\n", path);
#endif
    
    HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ,
                              nullptr, OPEN_EXISTING, 0, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        DBG("  CreateFileW failed: %lu\n", GetLastError());
        return false;
    }
    
    LARGE_INTEGER size;
    if (!GetFileSizeEx(file, &size)) {
        DBG("  GetFileSizeEx failed: %lu\n", GetLastError());
        CloseHandle(file);
        return false;
    }
    DBG_SIZE("File size", size.QuadPart);
    
    LARGE_INTEGER pos;
    pos.QuadPart = size.QuadPart - sizeof(clr::OverlayHeader);
    if (!SetFilePointerEx(file, pos, nullptr, FILE_BEGIN)) {
        DBG("  SetFilePointerEx failed: %lu\n", GetLastError());
        CloseHandle(file);
        return false;
    }
    
    DWORD read = 0;
    if (!ReadFile(file, &header, sizeof(header), &read, nullptr) ||
        read != sizeof(header)) {
        DBG("  ReadFile (header) failed: %lu\n", GetLastError());
        CloseHandle(file);
        return false;
    }
    
    DBG("  Header magic: 0x%08X\n", header.magic);
    DBG("  Expected:     0x%08X\n", clr::kOverlayMagic);
    
    if (header.magic != clr::kOverlayMagic) {
        DBG("  Magic mismatch! Invalid overlay.\n");
        CloseHandle(file);
        return false;
    }
    
    DBG_SIZE("Encrypted size", header.encrypted_size);
    
#ifdef DEBUG_BUILD
    printf("  Key: ");
    for (int i = 0; i < 8; i++) printf("%02X", header.key[i]);
    printf("...\n");
    printf("  IV:  ");
    for (int i = 0; i < 8; i++) printf("%02X", header.iv[i]);
    printf("...\n");
#endif
    
    pos.QuadPart = size.QuadPart - sizeof(clr::OverlayHeader) - header.encrypted_size;
    if (!SetFilePointerEx(file, pos, nullptr, FILE_BEGIN)) {
        DBG("  SetFilePointerEx (data) failed: %lu\n", GetLastError());
        CloseHandle(file);
        return false;
    }
    
    data.resize(header.encrypted_size);
    if (!ReadFile(file, data.data(), header.encrypted_size, &read, nullptr) ||
        read != header.encrypted_size) {
        DBG("  ReadFile (data) failed: %lu\n", GetLastError());
        CloseHandle(file);
        return false;
    }
    
    CloseHandle(file);
    DBG("  Overlay read successfully.\n\n");
    return true;
}

} // anonymous namespace

#ifdef DEBUG_BUILD
int main() {
    InitConsole();
#else
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
#endif
    
    DBG("Step 1: Reading overlay...\n");
    std::vector<uint8_t> encrypted;
    clr::OverlayHeader header;
    if (!ReadOverlay(encrypted, header)) {
        DBG("FAILED: Could not read overlay\n");
        WaitForExit();
        return 1;
    }
    DBG("OK: Overlay read (%zu bytes)\n\n", encrypted.size());
    
    DBG("Step 2: Decrypting assembly...\n");
    std::vector<uint8_t> assembly;
    if (!clr::crypto::Decrypt(encrypted, assembly, header.key, header.iv)) {
        DBG("FAILED: Decryption failed\n");
        WaitForExit();
        return 1;
    }
    DBG("OK: Decrypted (%zu bytes)\n\n", assembly.size());
    
#ifdef DEBUG_BUILD
    printf("  First 16 bytes: ");
    for (size_t i = 0; i < 16 && i < assembly.size(); i++) {
        printf("%02X ", assembly[i]);
    }
    printf("\n");
    
    if (assembly.size() >= 2 && assembly[0] == 'M' && assembly[1] == 'Z') {
        printf("  Valid PE header detected (MZ)\n\n");
    } else {
        printf("  WARNING: No MZ header - may not be valid PE\n\n");
    }
#endif
    
    SecureZeroMemory(encrypted.data(), encrypted.size());
    
    DBG("Step 3: Initializing CLR...\n");
    if (!clr::Initialize()) {
        DBG("FAILED: CLR initialization failed\n");
        WaitForExit();
        return 1;
    }
    DBG("OK: CLR initialized\n\n");
    
    DBG("Step 4: Executing assembly...\n");
    DBG("==========================================\n\n");
    
    bool success = clr::Execute(assembly);
    
#ifdef DEBUG_BUILD
    printf("\n==========================================\n");
    if (success) {
        printf("[DEBUG] Assembly executed successfully\n");
    } else {
        printf("[DEBUG] Assembly execution FAILED\n");
    }
#endif
    
    SecureZeroMemory(assembly.data(), assembly.size());
    clr::Shutdown();
    
    WaitForExit();
    return success ? 0 : 1;
}
