#pragma once

#include <charconv>
#include <stdexcept>
#include <string_view>
#include <type_traits>

namespace oryx::chron::details {

template <typename T>
inline auto StringCast(std::string_view sv) -> T {
    T value{};
    auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), value);
    if (ec != std::errc{}) throw std::invalid_argument("Invalid integer");
    return value;
}

template <typename T>
    requires std::is_enum_v<T>
inline auto StringCast(std::string_view sv) -> T {
    using _UT = std::underlying_type<T>;
    return static_cast<T>(StringCast<_UT>(sv));
}

}  // namespace oryx::chron::details