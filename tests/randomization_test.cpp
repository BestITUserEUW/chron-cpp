#include "doctest.hpp"

#include <string_view>

#include <oryx/chron/randomization.hpp>
#include <oryx/chron/scheduler.hpp>

using namespace oryx::chron;

namespace {

static Randomization rand{};
static Scheduler scheduler{};

template <bool ExpectFailure = false>
void Test(const std::string_view rand_expr) {
    for (auto i = 0; i < 5000; ++i) {
        auto cron_expr = rand.Parse(rand_expr);
        if constexpr (ExpectFailure) {
            // Parsing of random might succeed, but it yields an invalid schedule.
            REQUIRE_FALSE((cron_expr && scheduler.AddSchedule("validate schedule", *cron_expr, [](auto) {})));
        } else {
            REQUIRE(cron_expr);
            REQUIRE(scheduler.AddSchedule("validate schedule", *cron_expr, [](auto) {}));
        }
        scheduler.ClearSchedules();
    }
}

}  // namespace

TEST_CASE("Randomize all the things") {
    static constexpr std::string_view kRandomSchedule = "R(0-59) R(0-59) R(0-23) R(1-31) R(1-12) ?";

    SUBCASE(kRandomSchedule.data()) {
        THEN("Only valid schedules generated") { Test(kRandomSchedule); }
    }
}

TEST_CASE("Randomize all the things with reverse ranges") {
    static constexpr std::string_view kRandomSchedule = "R(45-15) R(30-0) R(18-2) R(28-15) R(8-3) ?";

    SUBCASE(kRandomSchedule.data()) {
        THEN("Only valid schedules generated") { Test(kRandomSchedule); }
    }
}

TEST_CASE("Randomize all the things - day of week") {
    static constexpr std::string_view kRandomSchedule = "R(0-59) R(0-59) R(0-23) ? R(1-12) R(0-6)";

    SUBCASE(kRandomSchedule.data()) {
        THEN("Only valid schedules generated") { Test(kRandomSchedule); }
    }
}

TEST_CASE("Randomize all the things with reverse ranges - day of week") {
    static constexpr std::string_view kRandomSchedule = "R(45-15) R(30-0) R(18-2) ? R(8-3) R(4-1)";

    SUBCASE(kRandomSchedule.data()) {
        THEN("Only valid schedules generated") { Test(kRandomSchedule); }
    }
}

TEST_CASE("Test readme examples") {
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

TEST_CASE("Randomization using text versions of days and months") {
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
            Test<true>("0 0 0 1 R(JAN-DEC) R(MON-SUN)");
        }
        AND_THEN("No schedule generated") {
            // Invalid range
            Test<true>("0 0 0 ? R(JAN) *");
        }
        AND_THEN("No schedule generated") {
            // Days in month field
            Test<true>("0 0 0 ? R(MON-TUE) *");
        }
        AND_THEN("No schedule generated") {
            // Month in day field
            Test<true>("0 0 0 ? * R(JAN-JUN)");
        }
    }
}
