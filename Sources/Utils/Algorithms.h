#pragma once

#include <functional>
namespace Rastery {
template <typename T, typename Compare>
void sort3(T& v0, T& v1, T& v2, Compare cmp) {
    if (cmp(v2, v1)) {
        std::swap(v2, v1);
    }
    if (cmp(v1, v0)) {
        std::swap(v1, v0);
    }
    if (cmp(v2, v1)) {
        std::swap(v2, v1);
    }
}

}  // namespace Rastery