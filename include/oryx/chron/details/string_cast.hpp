#pragma once

#include <charconv>
#include <cassert>
#include <string_view>

namespace oryx::chron::details {

template <typename T>
auto StringCast(std::string_view sv) -> T {
    T value{};
    auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), value);
    assert(ec == std::errc{} && "Casting to string failed misserably!");
    return value;
}

}  // namespace oryx::chron::details