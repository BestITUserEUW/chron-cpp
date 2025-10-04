#pragma once

#include <oryx/chron/data.hpp>
#include <oryx/chron/date_time.hpp>
#include <oryx/chron/chrono_types.hpp>

namespace oryx::chron {
class Schedule {
public:
    explicit Schedule(Data data)
        : data_(std::move(data)) {}

    auto CalculateFrom(const TimePoint& from) const -> std::tuple<bool, TimePoint>;

    static auto ToCalendarTime(TimePoint time) -> DateTime;

private:
    Data data_;
};

}  // namespace oryx::chron
