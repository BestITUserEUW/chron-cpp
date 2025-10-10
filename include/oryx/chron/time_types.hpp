#pragma once

#include <cstdint>

namespace oryx::chron {

enum class Seconds : uint8_t { First = 0, Last = 59 };

enum class Minutes : uint8_t { First = 0, Last = 59 };

enum class Hours : uint8_t { First = 0, Last = 23 };

enum class Days : uint8_t { First = 1, Last = 31 };

enum class Weeks : uint8_t {
    First = 0,
    Sunday = First,
    Monday,
    Tuesday,
    Wednesday,
    Thursday,
    Friday,
    Saturday = 6,
    Last = Saturday,
};

enum class Months : uint8_t {
    First = 1,
    January = First,
    February,
    March,
    April,
    May,
    June,
    July,
    August,
    September,
    October,
    November,
    December = 12,
    Last = December
};

}  // namespace oryx::chron
