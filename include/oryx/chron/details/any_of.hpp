#pragma once

#include <cstdint>
#include <set>

#include <oryx/chron/details/to_underlying.hpp>

namespace oryx::chron::details {

namespace traits {
template <typename T>
concept StaticCastableFromUInt8 = requires(uint8_t v) {
    { static_cast<T>(v) };
};
}  // namespace traits

template <traits::StaticCastableFromUInt8 T>
auto AnyOf(const std::set<T>& set, uint8_t low, uint8_t high) -> bool {
    for (auto i = low; i <= high; ++i)
        if (set.contains(static_cast<T>(i))) return true;
    return false;
}

template <typename Enum>
    requires std::is_enum_v<Enum>
auto AnyOf(const std::set<Enum>& set, Enum low, Enum high) -> bool {
    for (auto i = details::to_underlying(low); i <= details::to_underlying(high); ++i)
        if (set.contains(static_cast<Enum>(i))) return true;
    return false;
}

}  // namespace oryx::chron::details