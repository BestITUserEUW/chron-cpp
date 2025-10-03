
#include <oryx/chron/cron_clock.hpp>

#ifdef WIN32
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #define WIN32_LEAN_AND_MEAN
    #include <Windows.h>
#endif

using namespace std::chrono;

namespace oryx::chron {
std::chrono::seconds LocalClock::utc_offset(std::chrono::system_clock::time_point now) const {
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

bool TzClock::setTimezone(std::string_view name) {
    const std::chrono::time_zone *newZone{nullptr};

    try {
        newZone = std::chrono::locate_zone(name);
    } catch (std::runtime_error &err) {
        return false;
    }

    std::lock_guard<std::mutex> lock(_mtx);
    _timezone = newZone;
    return true;
}

std::chrono::seconds TzClock::utc_offset(std::chrono::system_clock::time_point now) const {
    using namespace std::chrono;
    // If we don't have a timezone set we use utc
    std::lock_guard<std::mutex> lock(_mtx);
    if (_timezone)
        return _timezone->get_info(now).offset;
    else
        return 0s;
}
}  // namespace oryx::chron