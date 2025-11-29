#ifndef CLR_LOADER_CLR_H
#define CLR_LOADER_CLR_H

#include <cstdint>
#include <vector>

namespace clr {

bool Initialize();
bool Execute(const std::vector<uint8_t>& assembly);
void Shutdown();

} // namespace clr

#endif // CLR_LOADER_CLR_H
