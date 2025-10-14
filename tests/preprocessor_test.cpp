#include "doctest.hpp"

#include <oryx/chron/preprocessor.hpp>

using namespace oryx::chron;

// TODO: Expand these tests

TEST_CASE("WeekMonthDayLiteralProcessor replaces correctly") {
    static constexpr std::string_view kExpr = "0 * * * JAN-feb MON-tue";

    auto schedule = WeekMonthDayLiteralProcessor::Process(std::string(kExpr));
    REQUIRE_EQ("0 * * * 1-2 1-2", schedule);
}