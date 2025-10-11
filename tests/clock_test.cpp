#include "doctest.hpp"

#include <oryx/chron/clock.hpp>

using namespace oryx::chron;
using namespace std::chrono;

TEST_CASE("TzClock Timezone is not set fallback to utc") {
    GIVEN("No timezone") {
        TzClock tz_clock{};
        auto now = Clock::now();
        REQUIRE(tz_clock.UtcOffset(now) == 0s);
    }
    GIVEN("A wrong timezone") {
        TzClock tz_clock{};
        auto now = Clock::now();
        tz_clock.TrySetTimezone("404Not/Found");
        REQUIRE(tz_clock.UtcOffset(now) == 0s);
    }
}

TEST_CASE("TzClock Setting time zone") {
    TzClock tz_clock{};
    GIVEN("Valid time zone") { REQUIRE(tz_clock.TrySetTimezone("Europe/Berlin")); }
    GIVEN("Invalid time zone") { REQUIRE_FALSE(tz_clock.TrySetTimezone("404Not/Found")); }
}