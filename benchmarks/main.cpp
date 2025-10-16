#define ANKERL_NANOBENCH_IMPLEMENT
#include "nanobench.hpp"

#include <oryx/chron/parser.hpp>
#include <oryx/chron/randomization.hpp>

#include <libcron/CronData.h>
#include <libcron/CronRandomization.h>

using namespace oryx::chron;
using namespace ankerl;

const std::string kRandomSchedule = "R(0-59) R(0-59) R(0-23) R(1-31) R(JAN-DEC) ?";

template <typename Parser>
void bench(ankerl::nanobench::Bench* bench, char const* name) {
    Parser parser;
    Randomization rng;

    bench->run(name, [&] { ankerl::nanobench::doNotOptimizeAway(parser(rng.Parse(kRandomSchedule).value()).value()); });
}

void bench(ankerl::nanobench::Bench* bench, char const* name) {
    Randomization rng;

    bench->run(name, [&] {
        ankerl::nanobench::doNotOptimizeAway(libcron::CronData::create(rng.Parse(kRandomSchedule).value()));
    });
}

auto main() -> int {
    static const auto kCachedParse = CachedExpressionParser();
    static const auto kMtx = CachedExpressionParser<std::mutex>();

    ankerl::nanobench::Bench b;
    b.title("Parsing randomized Expressions").epochIterations(20000).relative(true).performanceCounters(true);

    bench<ExpressionParser>(&b, "ExpressionParser");
    bench<CachedExpressionParser<>>(&b, "CachedExpressionParser<NullMutex>");
    bench<CachedExpressionParser<std::mutex>>(&b, "CachedExpressionParser<std::mutex>");
    bench(&b, "libcron::CronData::create");

    ankerl::nanobench::Bench b2;
    Randomization rng1;
    libcron::CronRandomization rng2;

    b2.title("Randomization").epochIterations(100000).relative(true).performanceCounters(true);
    b2.run("chron-cpp", [&] {
        auto r = rng1.Parse(kRandomSchedule);
        nanobench::doNotOptimizeAway(r);
    });
    b2.run("libcron", [&] {
        auto r = rng2.parse(kRandomSchedule);
        nanobench::doNotOptimizeAway(r);
    });
}