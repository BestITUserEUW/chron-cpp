# cron-cpp ![C++](https://img.shields.io/badge/c++-%2300599C.svg?style=for-the-badge&logo=c%2B%2B&logoColor=white)

**cron-cpp** is a C++20 scheduling library using cron formatting. 

This library is completely based on [libcron](https://github.com/PerMalmberg/libcron).

## Differences from libcron

- Proper CMake support
- date lib replaced with std::chrono implementation
- Time zone clock `TzClock`
- Automated tests for (clang, gcc, msvc)
- Correct Thread Safety Guarantees
- Optional Cron Expression Caching

## Third party libraries

- [ctre](https://github.com/hanickadot/compile-time-regular-expressions.git)

## Using the Scheduler

chron-cpp offers an easy to use API to add callbacks with corresponding cron-formatted strings:

```cpp
#include <thread>
#include <iostream>

#include <oryx/chron.hpp>

using namespace std::chrono_literals;

auto main() -> int {
    oryx::chron::Scheduler scheduler;

    scheduler.AddSchedule("Task-1", "* * * * * ?",
                          [](auto info) { std::cout << info.name << " called with delay " << info.delay << "\n"; });
    for (;;) {
        scheduler.Tick();
        std::this_thread::sleep_for(1s);
    }

    return 0;
}
```

In order to trigger execution of callbacks one must call `oryx::chron::Scheduler::Tick` at least once a second to prevent missing schedules:

```cpp
#include <thread>
#include <iostream>

#include <oryx/chron.hpp>

using namespace std::chrono_literals;

auto main() -> int {
    oryx::chron::Scheduler scheduler;

    scheduler.AddSchedule("Task-1", "* * * * * ?", [](auto info) {
        if (info.delay >= 1s) {
            std::cout << info.name << ": my scheduler is ticking to slow\n";
        }
    });
    for (;;) {
        scheduler.Tick();
        std::this_thread::sleep_for(2s);
    }

    return 0;
}
```

### Adding a batch of schedules at once

```cpp
#include <thread>
#include <iostream>
#include <cassert>

#include <oryx/chron.hpp>

using namespace std::chrono_literals;

auto main() -> int {
    oryx::chron::Scheduler scheduler;

    assert(scheduler.AddScheduleBatch([](auto add_schedule) {
        auto task = [](auto info) { std::cout << info.name << " was called\n"; };
        for (auto i = 0; i < 20; i++) {
            assert(add_schedule(std::to_string(i), "*/2 * * * * ?", task));
        }
    }));

    for (;;) {
        scheduler.Tick();
        std::this_thread::sleep_for(1s);
    }

    return 0;
}
```

### Removing schedules

`oryx::chron::Scheduler` offers two convenient functions to remove schedules:

- `ClearSchedules` will remove all schedules
- `RemoveSchedule` will remove a specific schedule

### ThreadSafe Scheduler

The scheduler by default is not thread safe if you need a thread safe Scheduler use `MTScheduler`. Alternatively you can also just drop in your own mutex like object. It just needs to satisfy the `traits::BasicLockable` concept.

```cpp
#include <atomic>
#include <csignal>
#include <string>
#include <thread>
#include <iostream>
#include <thread>

#include <oryx/chron.hpp>

using namespace std::chrono_literals;
using namespace oryx::chron;

std::atomic<bool> stop_requested{};

void Ticker(auto& scheduler) {
    while (!stop_requested) {
        scheduler.Tick();
        std::this_thread::sleep_for(50ms);
    }
}

auto main() -> int {
    signal(SIGINT, [](int sig) { stop_requested = true; });

    MTScheduler<UTCClock> scheduler{};
    std::thread worker{Ticker<decltype(scheduler)>, std::ref(scheduler)};
    uint64_t counter{};
    std::string task_name{};
    while (!stop_requested) {
        task_name = "Task-" + std::to_string(counter++);
        scheduler.AddSchedule(task_name, "* * * * * ?", [](TaskInfo info) { std::cout << info.name << ": Called\n"; });
        std::cout << "Scheduled: " << task_name << "\n";
        std::this_thread::sleep_for(50ms);
    }

    worker.join();
    std::cout << "Exiting\n";
    return 0;
}
```

### Caching

If you you are frequently parsing a lot of similar expressions you can speed up adding schedules by using one of the cached schedulers:

- `CScheduler` (Single Thread)
- `MTCScheduler` (Multi Thread)

## Scheduler Clock

The following clocks are available for the scheduler:

- `LocalClock` (default) offsets by operating systems time
- `UTCClock` offsets by 0
- `TzClock` offsets by 0 until a valid timezone has been set with `TrySetTimezone`

```cpp
#include <chrono>
#include <csignal>
#include <iostream>

#include <oryx/chron.hpp>

using namespace std::chrono;

auto main() -> int {
    oryx::chron::Scheduler<oryx::chron::TzClock> scheduler{};
    auto& clock = scheduler.GetClock();

    if (!clock.TrySetTimezone("Europe/Berlin")) {
        std::cout << "Failed to set timezone" << "\n";
        return 1;
    }

    std::cout << std::format("{:%Y-%m-%d %H:%M:%S}", std::chrono::floor<std::chrono::seconds>(clock.Now())) << '\n';
    return 0;
}
```

## Supported formatting

This implementation supports cron format, as specified below. 

Each schedule expression consists of 6 parts, all mandatory. However, if 'day of month' specifies specific days, then 'day of week' is ignored.

```text
┌──────────────seconds (0 - 59)
│ ┌───────────── minute (0 - 59)
│ │ ┌───────────── hour (0 - 23)
│ │ │ ┌───────────── day of month (1 - 31)
│ │ │ │ ┌───────────── month (1 - 12)
│ │ │ │ │ ┌───────────── day of week (0 - 6) (Sunday to Saturday)
│ │ │ │ │ │
│ │ │ │ │ │
│ │ │ │ │ │
* * * * * *
```
* Allowed formats:
  * Special characters: '*', meaning the entire range.
  * '?' used to ignore day of month/day of week as noted below.

  * Ranges: 1,2,4-6
    * Result: 1,2,4,5,6
  * Steps: n/m, where n is the start and m is the step.
    * `1/2` yields 1,3,5,7...<max>
    * `5/3` yields 5,8,11,14...<max>
    * `*/2` yields Result: 1,3,5,7...<max>
  * Reversed ranges:
    * `0 0 23-2 * * *`, meaning top of each minute and hour, of hours, 23, 0, 1 and 2, every day.
      * Compare to `0 0 2-23 * * *` which means top of each minute and hour, of hours, 2,3...21,22,23 every day.



For `month`, these (case insensitive) strings can be used instead of numbers: `JAN, FEB, MAR, APR, MAY, JUN, JUL, AUG, SEP, OCT, NOV, DEC`.
Example: `JAN,MAR,SEP-NOV`

For `day of week`, these (case insensitive) strings can be used instead of numbers: `SUN, MON, TUE, WED, THU, FRI, SAT`. 
Example: `MON-THU,SAT`

Each part is separated by one or more whitespaces. It is thus important to keep whitespaces out of the respective parts.

* Valid:
  * 0,3,40-50 * * * * ?

* Invalid:
  * 0, 3, 40-50 * * * * ?
  

`Day of month` and `day of week` are mutually exclusive so one of them must at always be ignored using
the '?'-character to ensure that it is not possible to specify a statement which results in an impossible mix of these fields. 

### Examples

|Expression | Meaning
| --- | --- |
| * * * * * ? | Every second
| 0 * * * * ? | Every minute
| 0 0 12 * * MON-FRI | Every Weekday at noon
| 0 0 12 1/2 * ?	| Every 2 days, starting on the 1st at noon
| 0 0 */12 ? * * | Every twelve hours
| @hourly | Every hour

Note that the expression formatting has a part for seconds and the day of week. 
For the day of week part, a question mark ? is utilized. This format
may not be parsed by all online crontab calculators or expression generators.

### Convenience scheduling

These special time specification tokens which replace the 5 initial time and date fields, and are prefixed with the '@' character, are supported:
|Token|Meaning
| --- | --- |
| @yearly | Run once a year, ie.  "0 0 0 1 1 *".
| @annually | Run once a year, ie.  "0 0 0 1 1 *".
| @monthly | Run once a month, ie. "0 0 0 1 * *".
| @weekly | Run once a week, ie.  "0 0 0 * * 0".
| @daily | Run once a day, ie.   "0 0 0 * * ?".
| @hourly | Run once an hour, ie. "0 0 * * * ?".
	
## Randomization

The standard cron format does not allow for randomization, but with the use of `oryx::chron::Randomization` you can generate random
schedules using the following format: `R(range_start-range_end)`, where `range_start` and `range_end` follow the same rules
as for a regular cron range (step-syntax is not supported). All the rules for a regular cron expression still applies
when using randomization, i.e. mutual exclusiveness and no extra spaces.

### Examples
|Expression | Meaning
| --- | --- |
| 0 0 R(13-20) * * ? | On the hour, on a random hour 13-20, inclusive.
| 0 0 0 ? * R(0-6) | A random weekday, every week, at midnight.
| 0 R(45-15) */12 ? * * | A random minute between 45-15, inclusive, every 12 hours.
|0 0 0 ? R(DEC-MAR) R(SAT-SUN)| On the hour, on a random month december to march, on a random weekday saturday to sunday. 

## Performance

These are some initial benchmarks on cron-parsing and cron randomization comparing against libcron:

OS: Linux
CPU: AMD Ryzen 9 7950X 16-Core Processor

| relative |               ns/op |                op/s |    err% |          ins/op |          cyc/op |    IPC |         bra/op |   miss% |     total | Parsing randomized Expressions
|---------:|--------------------:|--------------------:|--------:|----------------:|----------------:|-------:|---------------:|--------:|----------:|:-------------------------------
|   100.0% |          102,326.11 |            9,772.68 |    1.2% |    1,013,843.99 |      548,690.22 |  1.848 |     152,240.32 |    0.3% |     23.50 | `ExpressionParser`
|     9.2% |        1,116,211.84 |              895.89 |   40.5% |   16,046,595.84 |    5,985,076.41 |  2.681 |   2,785,616.84 |    0.0% |    247.82 | `CachedExpressionParser<NullMutex>`
|     9.3% |        1,103,677.48 |              906.06 |   40.3% |   16,042,923.93 |    5,952,656.76 |  2.695 |   2,785,005.43 |    0.1% |    245.24 | `CachedExpressionParser<std::mutex>`
|     7.9% |        1,298,631.94 |              770.04 |    0.2% |   13,982,049.29 |    7,033,908.92 |  1.988 |   2,239,027.96 |    0.4% |    285.72 | `libcron::CronData::create`

| relative |               ns/op |                op/s |    err% |          ins/op |          cyc/op |    IPC |         bra/op |   miss% |     total | Randomization
|---------:|--------------------:|--------------------:|--------:|----------------:|----------------:|-------:|---------------:|--------:|----------:|:--------------
|   100.0% |           85,454.44 |           11,702.14 |    0.5% |      879,301.99 |      463,006.51 |  1.899 |     134,616.91 |    0.3% |     93.98 | `chron-cpp`
|     5.7% |        1,493,531.23 |              669.55 |    0.2% |   16,608,204.87 |    8,084,464.40 |  2.054 |   2,685,597.92 |    0.4% |  1,643.85 | `libcron`

## Adding this library to your project

```cmake
include(FetchContent)

FetchContent_Declare(
    chron-cpp
    GIT_REPOSITORY https://github.com/BestITUserEUW/chron-cpp.git
    GIT_TAG main
    OVERRIDE_FIND_PACKAGE
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(chron-cpp)

find_package(chron-cpp REQUIRED)

target_link_libraries(my_project PUBLIC
    oryx::chron-cpp
)
```

## Build Locally

```bash
cmake -GNinja -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_BUILD_TYPE=Debug -Bbuild -H.
      ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
      Only needed for clangd   
```

```bash
cmake --build build -j32
```

## IDE Setup VsCode

### Clangd Extension (Recommended)

- Install clangd language server `apt install clangd`
- Go to Extension and install `llvm-vs-code-extensions.vscode-clangd`