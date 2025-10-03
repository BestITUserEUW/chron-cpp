#pragma once

#include <chrono>
#include <concepts>

namespace oryx::chron::traits {

template <typename T>
concept Clock = requires(const T& t, std::chrono::system_clock::time_point tp) {
    // Must provide: Now() -> std::chrono::system_clock::time_point
    { t.Now() } -> std::same_as<std::chrono::system_clock::time_point>;

    // Must provide: UtcOffset(time_point) -> std::chrono::seconds
    { t.UtcOffset(tp) } -> std::same_as<std::chrono::seconds>;
};

template <typename L>
concept BasicLockable = requires(L m) {
    m.lock();
    m.unlock();
};

}  // namespace oryx::chron::traits