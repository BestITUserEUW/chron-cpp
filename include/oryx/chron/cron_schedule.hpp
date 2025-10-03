#pragma once

#include <chrono>

#include "cron_data.hpp"
#include "date_time.hpp"

namespace oryx::chron {
class CronSchedule {
public:
    explicit CronSchedule(CronData& data)
        : data(data) {}

    CronSchedule(const CronSchedule&) = default;

    CronSchedule& operator=(const CronSchedule&) = default;

    std::tuple<bool, std::chrono::system_clock::time_point> calculate_from(
        const std::chrono::system_clock::time_point& from) const;

    // https://github.com/HowardHinnant/date/wiki/Examples-and-Recipes#obtaining-ymd-hms-components-from-a-time_point
    static DateTime to_calendar_time(std::chrono::system_clock::time_point time) {
        auto daypoint = std::chrono::floor<std::chrono::days>(time);
        auto ymd = std::chrono::year_month_day(daypoint);           // calendar date
        auto time_of_day = std::chrono::hh_mm_ss(time - daypoint);  // Yields time_of_day type

        // Obtain individual components as integers
        return {.year = int(ymd.year()),
                .month = unsigned(ymd.month()),
                .day = unsigned(ymd.day()),
                .hour = static_cast<uint8_t>(time_of_day.hours().count()),
                .min = static_cast<uint8_t>(time_of_day.minutes().count()),
                .sec = static_cast<uint8_t>(time_of_day.seconds().count())};
    }

private:
    CronData data;
};

}  // namespace oryx::chron
