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

template <typename Enum>
    requires std::is_enum_v<Enum>
auto AnyOf(const std::set<Enum>& range, Enum low, Enum high) -> bool {
    auto it = range.lower_bound(low);
    return it != range.end() && *it <= high;
}

template <traits::StaticCastableFromUInt8 T>
auto AnyOf(const std::set<T>& range, uint8_t low, uint8_t high) -> bool {
    return AnyOf(range, static_cast<T>(low), static_cast<T>(high));
}

}  // namespace oryx::chron::details