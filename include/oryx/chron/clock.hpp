#pragma once

#include <string_view>
#include <mutex>
#include <chrono>

#include "traits.hpp"

namespace oryx::chron {

class UTCClock {
public:
    auto Now() const -> std::chrono::system_clock::time_point { return std::chrono::system_clock::now(); }
    auto UtcOffset(std::chrono::system_clock::time_point) const -> std::chrono::seconds {
        using namespace std::chrono;
        return 0s;
    }
};

class LocalClock {
public:
    auto Now() const -> std::chrono::system_clock::time_point {
        auto now = std::chrono::system_clock::now();
        return now + UtcOffset(now);
    }

    auto UtcOffset(std::chrono::system_clock::time_point now) const -> std::chrono::seconds;
};

class TzClock {
public:
    auto Now() const -> std::chrono::system_clock::time_point {
        auto now = std::chrono::system_clock::now();
        return now + UtcOffset(now);
    }

    auto TrySetTimezone(std::string_view name) -> bool;
    auto UtcOffset(std::chrono::system_clock::time_point now) const -> std::chrono::seconds;

private:
    mutable std::mutex mtx_{};
    const std::chrono::time_zone* timezone_{};
};

static_assert(traits::Clock<LocalClock>);
static_assert(traits::Clock<UTCClock>);
static_assert(traits::Clock<TzClock>);

}  // namespace oryx::chron
