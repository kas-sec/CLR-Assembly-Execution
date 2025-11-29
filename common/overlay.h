#ifndef CLR_COMMON_OVERLAY_H
#define CLR_COMMON_OVERLAY_H

#include <cstdint>

namespace clr {

constexpr uint32_t kOverlayMagic = 0xDEADC0DE;
constexpr size_t kKeySize = 32;
constexpr size_t kIVSize  = 16;

#pragma pack(push, 1)
struct OverlayHeader {
    uint32_t magic;
    uint32_t encrypted_size;
    uint8_t  key[kKeySize];
    uint8_t  iv[kIVSize];
};
#pragma pack(pop)

} // namespace clr

#endif // CLR_COMMON_OVERLAY_H
