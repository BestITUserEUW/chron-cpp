#pragma once

#include <optional>

#include <oryx/chron/chron_data.hpp>
#include <oryx/chron/date_time.hpp>
#include <oryx/chron/chrono_types.hpp>

namespace oryx::chron {
class Schedule {
public:
    explicit Schedule(ChronData data)
        : data_(std::move(data)) {}

    auto CalculateFrom(const TimePoint& from) const -> std::optional<TimePoint>;

    static auto ToCalendarTime(TimePoint time) -> DateTime;

private:
    ChronData data_;
};

}  // namespace oryx::chron
