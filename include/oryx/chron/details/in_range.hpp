#pragma once

namespace oryx::chron::details {

template <std::totally_ordered T>
constexpr auto InRange(T value, T low, T high) -> bool {
    return value >= low && value <= high;
}

}  // namespace oryx::chron::details