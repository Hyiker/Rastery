#include "Color.h"

namespace Rastery {

float3 pseudoColor(uint32_t v) {
    uint32_t seed = v * 9386983906464221u + 2504963420354851u;
    return float3(seed & 0xff, seed >> 8 & 0xff, seed >> 16 & 0xff) / 255.f;
}
}  // namespace Rastery