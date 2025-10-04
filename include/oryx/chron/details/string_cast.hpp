#pragma once

#include <charconv>
#include <stdexcept>
#include <string_view>

namespace oryx::chron::details {

template <typename T>
inline auto StringCast(std::string_view sv) -> T {
    T value{};
    auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), value);
    if (ec != std::errc{}) throw std::invalid_argument("Invalid integer");
    return value;
};

}  // namespace oryx::chron::details