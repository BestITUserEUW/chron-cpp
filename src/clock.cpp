#include <oryx/chron/clock.hpp>

#ifdef WIN32
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #define WIN32_LEAN_AND_MEAN
    #include <Windows.h>
#endif

using namespace std::chrono;

namespace oryx::chron {

auto LocalClock::UtcOffset(TimePoint now) const -> seconds {
#ifdef WIN32
    (void)now;

    TIME_ZONE_INFORMATION tz_info{};
    seconds offset{0};

    auto res = GetTimeZoneInformation(&tz_info);
    if (res != TIME_ZONE_ID_INVALID) {
        // https://msdn.microsoft.com/en-us/library/windows/desktop/ms725481(v=vs.85).aspx
        // UTC = local time + bias => local_time = utc - bias, so UTC offset is -bias
        offset = minutes{-tz_info.Bias};
    }
#else
    auto t = system_clock::to_time_t(now);
    tm tm{};
    localtime_r(&t, &tm);
    seconds offset{tm.tm_gmtoff};
#endif
    return offset;
}

auto TzClock::TrySetTimezone(std::string_view name) -> bool {
    const time_zone *new_zone{};

    try {
        new_zone = locate_zone(name);
    } catch (std::runtime_error &err) {
        return false;
    }

    if (!new_zone) return false;

    std::lock_guard lock(mtx_);
    timezone_ = new_zone;
    return true;
}

auto TzClock::UtcOffset(TimePoint now) const -> seconds {
    // If we don't have a timezone set we use utc
    std::lock_guard lock(mtx_);
    if (timezone_)
        return timezone_->get_info(now).offset;
    else
        return 0s;
}
}  // namespace oryx::chron