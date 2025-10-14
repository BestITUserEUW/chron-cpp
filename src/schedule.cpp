#include <optional>

#include <oryx/chron/schedule.hpp>
#include <oryx/chron/details/to_underlying.hpp>
#include <oryx/chron/chrono_types.hpp>

using namespace std::chrono;

namespace oryx::chron {

auto Schedule::CalculateFrom(const TimePoint& from) const -> std::optional<TimePoint> {
    auto curr = from;

    bool done{};
    auto max_iterations = std::numeric_limits<uint16_t>::max();

    while (!done && --max_iterations > 0) {
        bool date_changed{};
        year_month_day ymd = floor<days>(curr);

        // Add months until one of the allowed days are found, or stay at the current one.
        if (!data_.months.contains(static_cast<Months>(unsigned(ymd.month())))) {
            auto next_month = ymd + months{1};
            sys_days s = next_month.year() / next_month.month() / 1;
            curr = s;
            date_changed = true;
        }
        // If all days are allowed (or the field is ignored via '?'), then the 'day of week' takes precedence.
        else if (data_.days.size() != details::to_underlying(MonthDays::Last)) {
            // Add days until one of the allowed days are found, or stay at the current one.
            if (!data_.days.contains(static_cast<MonthDays>(unsigned(ymd.day())))) {
                sys_days s = ymd;
                curr = s;
                curr += days{1};
                date_changed = true;
            }
        } else {
            // Add days until the current weekday is one of the allowed weekdays
            year_month_weekday ymw = floor<days>(curr);

            if (!data_.weeks.contains(static_cast<Weekdays>(ymw.weekday().c_encoding()))) {
                sys_days s = ymd;
                curr = s;
                curr += days{1};
                date_changed = true;
            }
        }

        if (!date_changed) {
            auto date_time = ToCalendarTime(curr);
            if (!data_.hours.contains(static_cast<Hours>(date_time.hour))) {
                curr += hours{1};
                curr -= minutes{date_time.min};
                curr -= seconds{date_time.sec};
            } else if (!data_.minutes.contains(static_cast<Minutes>(date_time.min))) {
                curr += minutes{1};
                curr -= seconds{date_time.sec};
            } else if (!data_.seconds.contains(static_cast<Seconds>(date_time.sec))) {
                curr += seconds{1};
            } else {
                done = true;
            }
        }
    }

    // Discard fraction seconds in the calculated schedule time
    //  that may leftover from the argument `from`, which in turn comes from `now()`.
    // Fraction seconds will potentially make the task be triggered more than 1 second late
    //  if the `tick()` within the same second is earlier than schedule time,
    //  in that the task will not trigger until the next `tick()` next second.
    // By discarding fraction seconds in the scheduled time,
    //  the `tick()` within the same second will never be earlier than schedule time,
    //  and the task will trigger in that `tick()`.
    curr -= curr.time_since_epoch() % seconds{1};
    if (max_iterations > 0) {
        return curr;
    }
    return std::nullopt;
}

auto Schedule::ToCalendarTime(TimePoint time) -> DateTime {
    auto daypoint = floor<days>(time);
    auto ymd = year_month_day(daypoint);           // calendar date
    auto time_of_day = hh_mm_ss(time - daypoint);  // Yields time_of_day type

    // Obtain individual components as integers
    return {.year = int(ymd.year()),
            .month = unsigned(ymd.month()),
            .day = unsigned(ymd.day()),
            .hour = static_cast<uint8_t>(time_of_day.hours().count()),
            .min = static_cast<uint8_t>(time_of_day.minutes().count()),
            .sec = static_cast<uint8_t>(time_of_day.seconds().count())};
}

}  // namespace oryx::chron