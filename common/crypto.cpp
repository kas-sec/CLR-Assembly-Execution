#include "crypto.h"

#include <windows.h>
#include <bcrypt.h>

#pragma comment(lib, "bcrypt.lib")

namespace clr {
namespace crypto {

bool RandomBytes(uint8_t* buffer, size_t size) {
    BCRYPT_ALG_HANDLE alg = nullptr;
    
    NTSTATUS status = BCryptOpenAlgorithmProvider(
        &alg, BCRYPT_RNG_ALGORITHM, nullptr, 0);
    
    if (!BCRYPT_SUCCESS(status)) return false;
    
    status = BCryptGenRandom(alg, buffer, static_cast<ULONG>(size), 0);
    BCryptCloseAlgorithmProvider(alg, 0);
    
    return BCRYPT_SUCCESS(status);
}

namespace {

class AESContext {
public:
    AESContext() : alg_(nullptr), key_(nullptr) {}
    
    ~AESContext() {
        if (key_) BCryptDestroyKey(key_);
        if (alg_) BCryptCloseAlgorithmProvider(alg_, 0);
    }
    
    bool Init(const uint8_t* key_data) {
        NTSTATUS status = BCryptOpenAlgorithmProvider(
            &alg_, BCRYPT_AES_ALGORITHM, nullptr, 0);
        if (!BCRYPT_SUCCESS(status)) return false;
        
        status = BCryptSetProperty(
            alg_, BCRYPT_CHAINING_MODE,
            (PBYTE)BCRYPT_CHAIN_MODE_CBC,
            sizeof(BCRYPT_CHAIN_MODE_CBC), 0);
        if (!BCRYPT_SUCCESS(status)) return false;
        
        status = BCryptGenerateSymmetricKey(
            alg_, &key_, nullptr, 0,
            (PBYTE)key_data, 32, 0);
        
        return BCRYPT_SUCCESS(status);
    }
    
    BCRYPT_KEY_HANDLE Key() const { return key_; }
    
private:
    BCRYPT_ALG_HANDLE alg_;
    BCRYPT_KEY_HANDLE key_;
};

} // anonymous namespace

bool Encrypt(const std::vector<uint8_t>& plaintext,
             std::vector<uint8_t>& ciphertext,
             const uint8_t* key,
             const uint8_t* iv) {
    AESContext ctx;
    if (!ctx.Init(key)) return false;
    
    std::vector<uint8_t> iv_copy(iv, iv + 16);
    DWORD size = 0;
    
    NTSTATUS status = BCryptEncrypt(
        ctx.Key(),
        (PBYTE)plaintext.data(), (ULONG)plaintext.size(),
        nullptr, iv_copy.data(), 16,
        nullptr, 0, &size, BCRYPT_BLOCK_PADDING);
    if (!BCRYPT_SUCCESS(status)) return false;
    
    ciphertext.resize(size);
    iv_copy.assign(iv, iv + 16);
    
    status = BCryptEncrypt(
        ctx.Key(),
        (PBYTE)plaintext.data(), (ULONG)plaintext.size(),
        nullptr, iv_copy.data(), 16,
        ciphertext.data(), size, &size, BCRYPT_BLOCK_PADDING);
    
    return BCRYPT_SUCCESS(status);
}

bool Decrypt(const std::vector<uint8_t>& ciphertext,
             std::vector<uint8_t>& plaintext,
             const uint8_t* key,
             const uint8_t* iv) {
    AESContext ctx;
    if (!ctx.Init(key)) return false;
    
    std::vector<uint8_t> iv_copy(iv, iv + 16);
    DWORD size = 0;
    
    NTSTATUS status = BCryptDecrypt(
        ctx.Key(),
        (PBYTE)ciphertext.data(), (ULONG)ciphertext.size(),
        nullptr, iv_copy.data(), 16,
        nullptr, 0, &size, BCRYPT_BLOCK_PADDING);
    if (!BCRYPT_SUCCESS(status)) return false;
    
    plaintext.resize(size);
    iv_copy.assign(iv, iv + 16);
    
    status = BCryptDecrypt(
        ctx.Key(),
        (PBYTE)ciphertext.data(), (ULONG)ciphertext.size(),
        nullptr, iv_copy.data(), 16,
        plaintext.data(), size, &size, BCRYPT_BLOCK_PADDING);
    
    if (BCRYPT_SUCCESS(status)) {
        plaintext.resize(size);
    }
    
    return BCRYPT_SUCCESS(status);
}

} // namespace crypto
} // namespace clr
