#pragma once

#include <type_traits>

namespace oryx::chron::details {

template <typename T>
[[nodiscard]]
constexpr auto to_underlying(T __value) noexcept -> std::underlying_type_t<T> {
    return static_cast<std::underlying_type_t<T>>(__value);
}

}  // namespace oryx::chron::details