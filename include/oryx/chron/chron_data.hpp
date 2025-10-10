#pragma once

#include <set>

#include "time_types.hpp"

namespace oryx::chron {

struct ChronData {
    ChronData() = default;

    std::set<Seconds> seconds;
    std::set<Minutes> minutes;
    std::set<Hours> hours;
    std::set<Days> days;
    std::set<Weeks> weeks;
    std::set<Months> months;
};

}  // namespace oryx::chron