#pragma once

#include <cstdint>

namespace oryx::chron {

struct DateTime {
    int year = 0;
    unsigned month = 0;
    unsigned day = 0;
    uint8_t hour = 0;
    uint8_t min = 0;
    uint8_t sec = 0;
};

}  // namespace oryx::chron
