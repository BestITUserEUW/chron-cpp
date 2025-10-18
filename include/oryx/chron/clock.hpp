#pragma once

#include <string_view>
#include <mutex>
#include <chrono>

#include "common.hpp"
#include "traits.hpp"

namespace oryx::chron {

class ORYX_CHRON_API UTCClock {
public:
    auto Now() const -> TimePoint { return Clock::now(); }
    auto UtcOffset(TimePoint) const -> std::chrono::seconds { return std::chrono::seconds(0); }
};

class ORYX_CHRON_API LocalClock {
public:
    auto Now() const -> TimePoint {
        auto now = Clock::now();
        return now + UtcOffset(now);
    }

    auto UtcOffset(TimePoint now) const -> std::chrono::seconds;
};

class ORYX_CHRON_API TzClock {
public:
    auto Now() const -> TimePoint {
        auto now = Clock::now();
        return now + UtcOffset(now);
    }

    auto TrySetTimezone(std::string_view name) -> bool;
    auto UtcOffset(TimePoint now) const -> std::chrono::seconds;

private:
    mutable std::mutex mtx_{};
    const void* timezone_{};
};

static_assert(traits::Clock<LocalClock>);
static_assert(traits::Clock<UTCClock>);
static_assert(traits::Clock<TzClock>);

}  // namespace oryx::chron
