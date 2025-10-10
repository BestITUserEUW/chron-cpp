#include "doctest.hpp"

#include <oryx/chron/details/time_types.hpp>
#include <oryx/chron/scheduler.hpp>
#include <oryx/chron/parser.hpp>
#include <oryx/chron/details/any_of.hpp>
#include <oryx/chron/details/all_of.hpp>

using namespace oryx::chron;
using namespace std::chrono;

SCENARIO("Numerical inputs") {
    GIVEN("Valid numerical inputs") {
        WHEN("Creating with all stars") {
            THEN("All parts are filled") {
                auto data = kParseExpression("* * * * * ?");
                REQUIRE(data.has_value());
                REQUIRE(data->seconds.size() == 60);
                REQUIRE(details::AllOf(data->seconds, 0, 59));
                REQUIRE(data->minutes.size() == 60);
                REQUIRE(details::AllOf(data->minutes, 0, 59));
                REQUIRE(data->hours.size() == 24);
                REQUIRE(details::AllOf(data->hours, 0, 23));
                REQUIRE(data->days.size() == 31);
                REQUIRE(details::AllOf(data->days, 1, 31));
                REQUIRE(data->weeks.size() == 7);
                REQUIRE(details::AllOf(data->weeks, 0, 6));
            }
        }
        AND_WHEN("Using full forward range") {
            THEN("Ranges are correct") {
                auto data = kParseExpression("* 0-59 * * * ?");
                REQUIRE(data.has_value());
                REQUIRE(data->seconds.size() == 60);
                REQUIRE(data->minutes.size() == 60);
                REQUIRE(data->hours.size() == 24);
                REQUIRE(data->days.size() == 31);
                REQUIRE(data->weeks.size() == 7);
                REQUIRE(details::AllOf(data->seconds, 0, 59));
            }
        }
        AND_WHEN("Using partial range") {
            THEN("Ranges are correct") {
                auto data = kParseExpression("* * * 20-30 * ?");
                REQUIRE(data.has_value());
                REQUIRE(data->seconds.size() == 60);
                REQUIRE(data->minutes.size() == 60);
                REQUIRE(data->hours.size() == 24);
                REQUIRE(data->days.size() == 11);
                REQUIRE(data->weeks.size() == 7);
                REQUIRE(details::AllOf(data->days, 20, 30));
            }
        }
        AND_WHEN("Using backward range") {
            THEN("Number of hours are correct") {
                auto data = kParseExpression("* * 20-5 * * ?");
                REQUIRE(data.has_value());
                REQUIRE(data->hours.size() == 10);
                REQUIRE(data->hours.find(Hours::First) != data->hours.end());
            }
        }
        AND_WHEN("Using various ranges") {
            THEN("Validation succeeds") {
                REQUIRE(kParseExpression("0-59 * * * * ?"));
                REQUIRE(kParseExpression("* 0-59 * * * ?"));
                REQUIRE(kParseExpression("* * 0-23 * * ?"));
                REQUIRE(kParseExpression("* * * 1-31 * ?"));
                REQUIRE(kParseExpression("* * * * 1-12 ?"));
                REQUIRE(kParseExpression("* * * ? * 0-6"));
            }
        }
    }
    GIVEN("Invalid inputs") {
        WHEN("Creating items") {
            THEN("Validation fails") {
                REQUIRE_FALSE(kParseExpression(""));
                REQUIRE_FALSE(kParseExpression("-"));
                REQUIRE_FALSE(kParseExpression("* "));
                REQUIRE_FALSE(kParseExpression("* 0-60 * * * ?"));
                REQUIRE_FALSE(kParseExpression("* * 0-25 * * ?"));
                REQUIRE_FALSE(kParseExpression("* * * 1-32 * ?"));
                REQUIRE_FALSE(kParseExpression("* * * * 1-13 ?"));
                REQUIRE_FALSE(kParseExpression("* * * * * 0-7"));
                REQUIRE_FALSE(kParseExpression("* * * 0-31 * ?"));
                REQUIRE_FALSE(kParseExpression("* * * * 0-12 ?"));
                REQUIRE_FALSE(kParseExpression("60 * * * * ?"));
                REQUIRE_FALSE(kParseExpression("* 60 * * * ?"));
                REQUIRE_FALSE(kParseExpression("* * 25 * * ?"));
                REQUIRE_FALSE(kParseExpression("* * * 32 * ?"));
                REQUIRE_FALSE(kParseExpression("* * * * 13 ?"));
                REQUIRE_FALSE(kParseExpression("* * * ? * 7"));
            }
        }
    }
}

SCENARIO("Literal input") {
    GIVEN("Literal inputs") {
        WHEN("Using literal ranges") {
            THEN("Range is valid") {
                auto data = kParseExpression("* * * * JAN-MAR ?");
                REQUIRE(data.has_value());
                REQUIRE(details::AllOf(data->months, 1, 3));
            }
            AND_THEN("Range is valid") {
                auto data = kParseExpression("* * * ? * SUN-FRI");
                REQUIRE(data.has_value());
                REQUIRE(details::AllOf(data->weeks, 0, 5));
            }
        }
        AND_WHEN("Using both range and specific month") {
            THEN("Range is valid") {
                auto data = kParseExpression("* * * * JAN-MAR,DEC ?");
                REQUIRE(data.has_value());
                REQUIRE(details::AllOf(data->months, 1, 3));
                REQUIRE_FALSE(details::AnyOf(data->months, 4, 11));
                REQUIRE(details::AllOf(data->months, 12, 12));
            }
            AND_THEN("Range is valid") {
                auto data = kParseExpression("* * * ? JAN-MAR,DEC FRI,MON,THU");
                REQUIRE(data.has_value());
                REQUIRE(details::AllOf(data->months, 1, 3));
                REQUIRE_FALSE(details::AnyOf(data->months, 4, 11));
                REQUIRE(details::AllOf(data->months, 12, 12));
                REQUIRE(details::AllOf(data->weeks, 5, 5));
                REQUIRE(details::AllOf(data->weeks, 1, 1));
                REQUIRE(details::AllOf(data->weeks, 4, 4));
                REQUIRE_FALSE(details::AnyOf(data->weeks, 0, 0));
                REQUIRE_FALSE(details::AnyOf(data->weeks, 2, 3));
                REQUIRE_FALSE(details::AnyOf(data->weeks, 6, 6));
            }
        }
        AND_WHEN("Using backward range") {
            THEN("Range is valid") {
                auto data = kParseExpression("* * * ? APR-JAN *");
                REQUIRE(data.has_value());
                REQUIRE(details::AllOf(data->months, 4, 12));
                REQUIRE(details::AllOf(data->months, 1, 1));
                REQUIRE_FALSE(details::AnyOf(data->months, 2, 3));
            }
            AND_THEN("Range is valid") {
                auto data = kParseExpression("* * * ? * sat-tue,wed");
                REQUIRE(data.has_value());
                REQUIRE(details::AllOf(data->weeks, 6, 6));        // Has saturday
                REQUIRE(details::AllOf(data->weeks, 0, 3));        // Has sun, mon, tue, wed
                REQUIRE_FALSE(details::AnyOf(data->weeks, 4, 5));  // Does not have thu or fri.
            }
        }
    }
}

SCENARIO("Parsing @ expressions works") {
    REQUIRE(kParseExpression("@yearly"));
    REQUIRE(kParseExpression("@annually"));
    REQUIRE(kParseExpression("@monthly"));
    REQUIRE(kParseExpression("@weekly"));
    REQUIRE(kParseExpression("@daily"));
    REQUIRE(kParseExpression("@hourly"));
}

SCENARIO("Using step syntax") {
    GIVEN("Step inputs") {
        WHEN("Using literal ranges") {
            THEN("Range is valid") {
                auto data = kParseExpression("* * * * JAN/2 ?");
                REQUIRE(data.has_value());
                REQUIRE(details::AllOf(data->months, 1, 1));
                REQUIRE(details::AllOf(data->months, 3, 3));
                REQUIRE(details::AllOf(data->months, 5, 5));
                REQUIRE(details::AllOf(data->months, 7, 7));
                REQUIRE(details::AllOf(data->months, 9, 9));
                REQUIRE(details::AllOf(data->months, 11, 11));
                REQUIRE_FALSE(details::AnyOf(data->months, 2, 2));
                REQUIRE_FALSE(details::AnyOf(data->months, 4, 4));
                REQUIRE_FALSE(details::AnyOf(data->months, 6, 6));
                REQUIRE_FALSE(details::AnyOf(data->months, 8, 8));
                REQUIRE_FALSE(details::AnyOf(data->months, 10, 10));
                REQUIRE_FALSE(details::AnyOf(data->months, 12, 12));
            }
        }
    }
}

SCENARIO("Dates that does not exist") {
    REQUIRE_FALSE(kParseExpression("0 0 * 30 FEB *"));
    REQUIRE_FALSE(kParseExpression("0 0 * 31 APR *"));
}

SCENARIO("Date that exist in one of the months") { REQUIRE(kParseExpression("0 0 * 31 APR,MAY ?")); }

SCENARIO("Replacing text with numbers") {
    {
        std::string s = "SUN-TUE";
        REQUIRE(details::ReplaceDayNameWithNumeric(s) == "0-2");
    }

    {
        std::string s = "JAN-DEC";
        REQUIRE(details::ReplaceMonthNameWithNumeric(s) == "1-12");
    }
}