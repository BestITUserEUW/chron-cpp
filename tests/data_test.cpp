#include "doctest.hpp"

#include <oryx/chron/scheduler.hpp>
#include <oryx/chron/data.hpp>

using namespace oryx::chron;
using namespace std::chrono;

namespace {

template <typename T>
auto HasValueRange(const std::set<T>& set, uint8_t low, uint8_t high) -> bool {
    bool found = true;
    for (auto i = low; found && i <= high; ++i) {
        found &= set.find(static_cast<T>(i)) != set.end();
    }

    return found;
}

}  // namespace

SCENARIO("Numerical inputs") {
    GIVEN("Valid numerical inputs") {
        WHEN("Creating with all stars") {
            THEN("All parts are filled") {
                auto c = Data::Create("* * * * * ?");
                REQUIRE(c.IsValid());
                REQUIRE(c.GetSeconds().size() == 60);
                REQUIRE(HasValueRange(c.GetSeconds(), 0, 59));
                REQUIRE(c.GetMinutes().size() == 60);
                REQUIRE(HasValueRange(c.GetMinutes(), 0, 59));
                REQUIRE(c.GetHours().size() == 24);
                REQUIRE(HasValueRange(c.GetHours(), 0, 23));
                REQUIRE(c.GetDayOfMonth().size() == 31);
                REQUIRE(HasValueRange(c.GetDayOfMonth(), 1, 31));
                REQUIRE(c.GetDayOfWeek().size() == 7);
                REQUIRE(HasValueRange(c.GetDayOfWeek(), 0, 6));
            }
        }
        AND_WHEN("Using full forward range") {
            THEN("Ranges are correct") {
                auto c = Data::Create("* 0-59 * * * ?");
                REQUIRE(c.IsValid());
                REQUIRE(c.GetSeconds().size() == 60);
                REQUIRE(c.GetMinutes().size() == 60);
                REQUIRE(c.GetHours().size() == 24);
                REQUIRE(c.GetDayOfMonth().size() == 31);
                REQUIRE(c.GetDayOfWeek().size() == 7);
                REQUIRE(HasValueRange(c.GetSeconds(), 0, 59));
            }
        }
        AND_WHEN("Using partial range") {
            THEN("Ranges are correct") {
                auto c = Data::Create("* * * 20-30 * ?");
                REQUIRE(c.IsValid());
                REQUIRE(c.GetSeconds().size() == 60);
                REQUIRE(c.GetMinutes().size() == 60);
                REQUIRE(c.GetHours().size() == 24);
                REQUIRE(c.GetDayOfMonth().size() == 11);
                REQUIRE(c.GetDayOfWeek().size() == 7);
                REQUIRE(HasValueRange(c.GetDayOfMonth(), 20, 30));
            }
        }
        AND_WHEN("Using backward range") {
            THEN("Number of hours are correct") {
                auto c = Data::Create("* * 20-5 * * ?");
                REQUIRE(c.IsValid());
                REQUIRE(c.GetHours().size() == 10);
                REQUIRE(c.GetHours().find(Hours::First) != c.GetHours().end());
            }
        }
        AND_WHEN("Using various ranges") {
            THEN("Validation succeeds") {
                REQUIRE(Data::Create("0-59 * * * * ?").IsValid());
                REQUIRE(Data::Create("* 0-59 * * * ?").IsValid());
                REQUIRE(Data::Create("* * 0-23 * * ?").IsValid());
                REQUIRE(Data::Create("* * * 1-31 * ?").IsValid());
                REQUIRE(Data::Create("* * * * 1-12 ?").IsValid());
                REQUIRE(Data::Create("* * * ? * 0-6").IsValid());
            }
        }
    }
    GIVEN("Invalid inputs") {
        WHEN("Creating items") {
            THEN("Validation fails") {
                REQUIRE_FALSE(Data::Create("").IsValid());
                REQUIRE_FALSE(Data::Create("-").IsValid());
                REQUIRE_FALSE(Data::Create("* ").IsValid());
                REQUIRE_FALSE(Data::Create("* 0-60 * * * ?").IsValid());
                REQUIRE_FALSE(Data::Create("* * 0-25 * * ?").IsValid());
                REQUIRE_FALSE(Data::Create("* * * 1-32 * ?").IsValid());
                REQUIRE_FALSE(Data::Create("* * * * 1-13 ?").IsValid());
                REQUIRE_FALSE(Data::Create("* * * * * 0-7").IsValid());
                REQUIRE_FALSE(Data::Create("* * * 0-31 * ?").IsValid());
                REQUIRE_FALSE(Data::Create("* * * * 0-12 ?").IsValid());
                REQUIRE_FALSE(Data::Create("60 * * * * ?").IsValid());
                REQUIRE_FALSE(Data::Create("* 60 * * * ?").IsValid());
                REQUIRE_FALSE(Data::Create("* * 25 * * ?").IsValid());
                REQUIRE_FALSE(Data::Create("* * * 32 * ?").IsValid());
                REQUIRE_FALSE(Data::Create("* * * * 13 ?").IsValid());
                REQUIRE_FALSE(Data::Create("* * * ? * 7").IsValid());
            }
        }
    }
}

SCENARIO("Literal input") {
    GIVEN("Literal inputs") {
        WHEN("Using literal ranges") {
            THEN("Range is valid") {
                auto c = Data::Create("* * * * JAN-MAR ?");
                REQUIRE(c.IsValid());
                REQUIRE(HasValueRange(c.GetMonths(), 1, 3));
            }
            AND_THEN("Range is valid") {
                auto c = Data::Create("* * * ? * SUN-FRI");
                REQUIRE(c.IsValid());
                REQUIRE(HasValueRange(c.GetDayOfWeek(), 0, 5));
            }
        }
        AND_WHEN("Using both range and specific month") {
            THEN("Range is valid") {
                auto c = Data::Create("* * * * JAN-MAR,DEC ?");
                REQUIRE(c.IsValid());
                REQUIRE(HasValueRange(c.GetMonths(), 1, 3));
                REQUIRE_FALSE(Data::HasAnyInRange(c.GetMonths(), 4, 11));
                REQUIRE(HasValueRange(c.GetMonths(), 12, 12));
            }
            AND_THEN("Range is valid") {
                auto c = Data::Create("* * * ? JAN-MAR,DEC FRI,MON,THU");
                REQUIRE(c.IsValid());
                REQUIRE(HasValueRange(c.GetMonths(), 1, 3));
                REQUIRE_FALSE(Data::HasAnyInRange(c.GetMonths(), 4, 11));
                REQUIRE(HasValueRange(c.GetMonths(), 12, 12));
                REQUIRE(HasValueRange(c.GetDayOfWeek(), 5, 5));
                REQUIRE(HasValueRange(c.GetDayOfWeek(), 1, 1));
                REQUIRE(HasValueRange(c.GetDayOfWeek(), 4, 4));
                REQUIRE_FALSE(Data::HasAnyInRange(c.GetDayOfWeek(), 0, 0));
                REQUIRE_FALSE(Data::HasAnyInRange(c.GetDayOfWeek(), 2, 3));
                REQUIRE_FALSE(Data::HasAnyInRange(c.GetDayOfWeek(), 6, 6));
            }
        }
        AND_WHEN("Using backward range") {
            THEN("Range is valid") {
                auto c = Data::Create("* * * ? APR-JAN *");
                REQUIRE(c.IsValid());
                REQUIRE(HasValueRange(c.GetMonths(), 4, 12));
                REQUIRE(HasValueRange(c.GetMonths(), 1, 1));
                REQUIRE_FALSE(Data::HasAnyInRange(c.GetMonths(), 2, 3));
            }
            AND_THEN("Range is valid") {
                auto c = Data::Create("* * * ? * sat-tue,wed");
                REQUIRE(c.IsValid());
                REQUIRE(HasValueRange(c.GetDayOfWeek(), 6, 6));              // Has saturday
                REQUIRE(HasValueRange(c.GetDayOfWeek(), 0, 3));              // Has sun, mon, tue, wed
                REQUIRE_FALSE(Data::HasAnyInRange(c.GetDayOfWeek(), 4, 5));  // Does not have thu or fri.
            }
        }
    }
}

SCENARIO("paring @ expressions works") {
    REQUIRE(Data::Create("@yearly").IsValid());
    REQUIRE(Data::Create("@annually").IsValid());
    REQUIRE(Data::Create("@monthly").IsValid());
    REQUIRE(Data::Create("@weekly").IsValid());
    REQUIRE(Data::Create("@daily").IsValid());
    REQUIRE(Data::Create("@hourly").IsValid());
}

SCENARIO("Using step syntax") {
    GIVEN("Step inputs") {
        WHEN("Using literal ranges") {
            THEN("Range is valid") {
                auto c = Data::Create("* * * * JAN/2 ?");
                REQUIRE(c.IsValid());
                REQUIRE(HasValueRange(c.GetMonths(), 1, 1));
                REQUIRE(HasValueRange(c.GetMonths(), 3, 3));
                REQUIRE(HasValueRange(c.GetMonths(), 5, 5));
                REQUIRE(HasValueRange(c.GetMonths(), 7, 7));
                REQUIRE(HasValueRange(c.GetMonths(), 9, 9));
                REQUIRE(HasValueRange(c.GetMonths(), 11, 11));
                REQUIRE_FALSE(Data::HasAnyInRange(c.GetMonths(), 2, 2));
                REQUIRE_FALSE(Data::HasAnyInRange(c.GetMonths(), 4, 4));
                REQUIRE_FALSE(Data::HasAnyInRange(c.GetMonths(), 6, 6));
                REQUIRE_FALSE(Data::HasAnyInRange(c.GetMonths(), 8, 8));
                REQUIRE_FALSE(Data::HasAnyInRange(c.GetMonths(), 10, 10));
                REQUIRE_FALSE(Data::HasAnyInRange(c.GetMonths(), 12, 12));
            }
        }
    }
}

SCENARIO("Dates that does not exist") {
    REQUIRE_FALSE(Data::Create("0 0 * 30 FEB *").IsValid());
    REQUIRE_FALSE(Data::Create("0 0 * 31 APR *").IsValid());
}

SCENARIO("Date that exist in one of the months") { REQUIRE(Data::Create("0 0 * 31 APR,MAY ?").IsValid()); }

SCENARIO("Replacing text with numbers") {
    {
        std::string s = "SUN-TUE";
        REQUIRE(Data::ReplaceStringNameWithNumeric<DayOfWeek>(s) == "0-2");
    }

    {
        std::string s = "JAN-DEC";
        REQUIRE(Data::ReplaceStringNameWithNumeric<Months>(s) == "1-12");
    }
}