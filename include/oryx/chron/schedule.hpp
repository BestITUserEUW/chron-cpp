#pragma once

#include <optional>

#include "common.hpp"
#include "chron_data.hpp"
#include "date_time.hpp"

namespace oryx::chron {

class ORYX_CHRON_API Schedule {
public:
    explicit Schedule(ChronData data)
        : data_(std::move(data)) {}

    auto CalculateFrom(const TimePoint& from) const -> std::optional<TimePoint>;

    static auto ToCalendarTime(TimePoint time) -> DateTime;

private:
    ChronData data_;
};

}  // namespace oryx::chron
