#pragma once

#include <chrono>
#include <concepts>

#include "chrono_types.hpp"

namespace oryx::chron::traits {

template <typename T>
concept Clock = requires(const T& t, TimePoint tp) {
    { t.Now() } -> std::same_as<TimePoint>;
    { t.UtcOffset(tp) } -> std::same_as<std::chrono::seconds>;
};

template <typename L>
concept BasicLockable = requires(L m) {
    m.lock();
    m.unlock();
};

}  // namespace oryx::chron::traits