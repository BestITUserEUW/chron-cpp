#pragma once

#include <charconv>
#include <cassert>
#include <string_view>

namespace oryx::chron::details {

template <typename T>
auto StringCast(std::string_view sv) -> T {
    T value{};
    [[maybe_unused]] auto result = std::from_chars(sv.data(), sv.data() + sv.size(), value);
    assert(result.ec == std::errc{} && "Casting to string failed misserably!");
    return value;
}

}  // namespace oryx::chron::details