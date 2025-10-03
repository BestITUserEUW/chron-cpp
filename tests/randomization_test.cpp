#include "doctest.hpp"

#include <string>

#include <oryx/chron/randomization.hpp>
#include <oryx/chron/scheduler.hpp>

using namespace oryx::chron;

namespace {

void Test(const char* const random_schedule, bool expect_failure = false) {
    Randomization cr;

    for (int i = 0; i < 5000; ++i) {
        auto res = cr.Parse(random_schedule);
        auto schedule = std::get<1>(res);

        Scheduler scheduler;

        if (expect_failure) {
            // Parsing of random might succeed, but it yields an invalid schedule.
            auto r = std::get<0>(res) && scheduler.AddSchedule("validate schedule", schedule, [](auto&) {});
            REQUIRE_FALSE(r);
        } else {
            REQUIRE(std::get<0>(res));
            REQUIRE(scheduler.AddSchedule("validate schedule", schedule, [](auto&) {}));
        }
    }
}

}  // namespace

SCENARIO("Randomize all the things") {
    const char* random_schedule = "R(0-59) R(0-59) R(0-23) R(1-31) R(1-12) ?";

    SUBCASE(random_schedule) {
        THEN("Only valid schedules generated") { Test(random_schedule); }
    }
}

SCENARIO("Randomize all the things with reverse ranges") {
    const char* random_schedule = "R(45-15) R(30-0) R(18-2) R(28-15) R(8-3) ?";

    SUBCASE(random_schedule) {
        THEN("Only valid schedules generated") { Test(random_schedule); }
    }
}

SCENARIO("Randomize all the things - day of week") {
    const char* random_schedule = "R(0-59) R(0-59) R(0-23) ? R(1-12) R(0-6)";

    SUBCASE(random_schedule) {
        THEN("Only valid schedules generated") { Test(random_schedule); }
    }
}

SCENARIO("Randomize all the things with reverse ranges - day of week") {
    const char* random_schedule = "R(45-15) R(30-0) R(18-2) ? R(8-3) R(4-1)";

    SUBCASE(random_schedule) {
        THEN("Only valid schedules generated") { Test(random_schedule); }
    }
}

SCENARIO("Test readme examples") {
    SUBCASE("0 0 R(13-20) * * ?") {
        THEN("Valid schedule generated") { Test("0 0 R(13-20) * * ?"); }
    }

    SUBCASE("0 0 0 ? * R(0-6)") {
        THEN("Valid schedule generated") { Test("0 0 0 ? * R(0-6)"); }
    }

    SUBCASE("0 R(45-15) */12 ? * *") {
        THEN("Valid schedule generated") { Test("0 R(45-15) */12 ? * *"); }
    }
}

SCENARIO("Randomization using text versions of days and months") {
    SUBCASE("0 0 0 ? * R(TUE-FRI)") {
        THEN("Valid schedule generated") { Test("0 0 0 ? * R(TUE-FRI)"); }
    }

    SUBCASE("Valid schedule") {
        THEN("Valid schedule generated") { Test("0 0 0 ? R(JAN-DEC) R(MON-FRI)"); }
        AND_WHEN("SUBCASE 0 0 0 ? R(DEC-MAR) R(SAT-SUN)") {
            THEN("Valid schedule generated") { Test("0 0 0 ? R(DEC-MAR) R(SAT-SUN)"); }
        }
        AND_THEN("SUBCASE 0 0 0 ? R(JAN-FEB) *") {
            THEN("Valid schedule generated") { Test("0 0 0 ? R(JAN-FEB) *"); }
        }
        AND_THEN("SUBCASE 0 0 0 ? R(OCT-OCT) *") {
            THEN("Valid schedule generated") { Test("0 0 0 ? R(OCT-OCT) *"); }
        }
    }

    SUBCASE("Invalid schedule") {
        THEN("No schedule generated") {
            // Day of month specified - not allowed with day of week
            Test("0 0 0 1 R(JAN-DEC) R(MON-SUN)", true);
        }
        AND_THEN("No schedule generated") {
            // Invalid range
            Test("0 0 0 ? R(JAN) *", true);
        }
        AND_THEN("No schedule generated") {
            // Days in month field
            Test("0 0 0 ? R(MON-TUE) *", true);
        }
        AND_THEN("No schedule generated") {
            // Month in day field
            Test("0 0 0 ? * R(JAN-JUN)", true);
        }
    }
}
