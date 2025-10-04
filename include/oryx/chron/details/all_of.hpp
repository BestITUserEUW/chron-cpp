#pragma once

#include <cstdint>
#include <set>

namespace oryx::chron::details {

template <typename T>
auto AllOf(const std::set<T>& set, uint8_t low, uint8_t high) -> bool {
    bool found{true};
    for (auto i = low; found && i <= high; ++i) {
        found &= set.find(static_cast<T>(i)) != set.end();
    }
    return found;
}

}  // namespace oryx::chron::details