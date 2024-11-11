#pragma once

#include <fmt/format.h>

#include <span>
#include <type_traits>

#include "Utils/Logger.h"

namespace Rastery {
// Helper using ADL to find EnumInfo in other namespaces.
template <typename T>
using EnumInfo = decltype(rasteryFindEnumInfoADL(std::declval<T>()));

template <typename, typename = void>
struct has_enum_info : std::false_type {};

template <typename T>
struct has_enum_info<T, std::void_t<decltype(EnumInfo<T>::items)>> : std::true_type {};

template <typename T>
inline constexpr bool has_enum_info_v = has_enum_info<T>::value;

template <typename T, std::enable_if_t<has_enum_info_v<T>, bool> = true>
inline const std::string& enumToString(T value) {
    const auto& items = EnumInfo<T>::items();
    auto it = std::find_if(items.begin(), items.end(), [value](const auto& item) { return item.first == value; });
    if (it == items.end()) logFatal("Invalid enum value {}", int(value));
    return it->second;
}
}  // namespace Rastery

#define RASTERY_ENUM_FLAG(T)                                                                                          \
    inline T operator|(T a, T b) {                                                                                    \
        return static_cast<T>(static_cast<std::underlying_type_t<T>>(a) | static_cast<std::underlying_type_t<T>>(b)); \
    }                                                                                                                 \
    inline T operator&(T a, T b) {                                                                                    \
        return static_cast<T>(static_cast<std::underlying_type_t<T>>(a) & static_cast<std::underlying_type_t<T>>(b)); \
    }                                                                                                                 \
    inline T operator~(T a) { return static_cast<T>(~static_cast<std::underlying_type_t<T>>(a)); }                    \
    inline T& operator|=(T& a, T b) { return a = a | b; }                                                             \
    inline T& operator&=(T& a, T b) { return a = a & b; }                                                             \
    inline bool isSet(T a, T b) { return (a & b) == b; }

#define RASTERY_ENUM_INFO(T, ...)                                   \
    struct T##_info {                                               \
        static std::span<std::pair<T, std::string>> items() {       \
            static std::pair<T, std::string> items[] = __VA_ARGS__; \
            return {std::begin(items), std::end(items)};            \
        }                                                           \
    };

#define RASTERY_ENUM_REGISTER(T) \
    constexpr T##_info rasteryFindEnumInfoADL [[maybe_unused]] (T) noexcept { return T##_info{}; }

/// Enum formatter.
template <typename T>
struct fmt::formatter<T, std::enable_if_t<Rastery::has_enum_info_v<T>, char>> : formatter<std::string> {
    template <typename FormatContext>
    auto format(const T& e, FormatContext& ctx) {
        return formatter<std::string>::format(Rastery::enumToString(e), ctx);
    }
};