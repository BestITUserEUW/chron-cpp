#pragma once

#include <cstdint>

namespace oryx::chron {

enum class Seconds : int8_t { First = 0, Last = 59 };

enum class Minutes : int8_t { First = 0, Last = 59 };

enum class Hours : int8_t { First = 0, Last = 23 };

enum class DayOfMonth : uint8_t { First = 1, Last = 31 };

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

enum class DayOfWeek : uint8_t {
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
}  // namespace oryx::chron
