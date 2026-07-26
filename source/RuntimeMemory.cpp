#include <cstdint>

#if defined(__3DS__)
extern "C" {
std::uint32_t __stacksize__ = 128u * 1024u;
}
#endif
