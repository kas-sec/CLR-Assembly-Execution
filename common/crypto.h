#ifndef CLR_COMMON_CRYPTO_H
#define CLR_COMMON_CRYPTO_H

#include <cstdint>
#include <vector>

namespace clr {
namespace crypto {

bool RandomBytes(uint8_t* buffer, size_t size);

bool Encrypt(const std::vector<uint8_t>& plaintext,
             std::vector<uint8_t>& ciphertext,
             const uint8_t* key,
             const uint8_t* iv);

bool Decrypt(const std::vector<uint8_t>& ciphertext,
             std::vector<uint8_t>& plaintext,
             const uint8_t* key,
             const uint8_t* iv);

} // namespace crypto
} // namespace clr

#endif // CLR_COMMON_CRYPTO_H
