#pragma once

#include <chrono>

#include "data.hpp"
#include "date_time.hpp"

namespace oryx::chron {
class Schedule {
public:
    explicit Schedule(Data data)
        : data_(std::move(data)) {}

    Schedule(const Schedule&) = default;

    auto operator=(const Schedule&) -> Schedule& = default;

    auto CalculateFrom(const std::chrono::system_clock::time_point& from) const
        -> std::tuple<bool, std::chrono::system_clock::time_point>;

    static auto ToCalendarTime(std::chrono::system_clock::time_point time) -> DateTime;

private:
    Data data_;
};

}  // namespace oryx::chron
