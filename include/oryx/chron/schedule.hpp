#pragma once

#include <tuple>

#include "data.hpp"
#include "date_time.hpp"
#include "chrono_types.h"

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
