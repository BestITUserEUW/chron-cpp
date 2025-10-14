#pragma once

#include <cstdint>
#include <array>

namespace oryx::chron {

enum class Seconds : uint8_t { First = 0, Last = 59 };

enum class Minutes : uint8_t { First = 0, Last = 59 };

enum class Hours : uint8_t { First = 0, Last = 23 };

enum class MonthDays : uint8_t { First = 1, Last = 31 };

enum class Weekdays : uint8_t {
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

inline constexpr std::array<Months, 7> kMonthsWith31{Months::January, Months::March,   Months::May,     Months::July,
                                                     Months::August,  Months::October, Months::December};

}  // namespace oryx::chron
