# cron-cpp ![C++](https://img.shields.io/badge/c++-%2300599C.svg?style=for-the-badge&logo=c%2B%2B&logoColor=white)

**cron-cpp** is a C++20 scheduling library using cron formatting. 

This library is completely based on [libcron](https://github.com/PerMalmberg/libcron).

## Differences from libcron

- Proper CMake support
- date lib replaced with std::chrono implementation
- Time zone clock
- Automated tests for (clang, gcc, msvc)

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

    scheduler.AddSchedule("Task-1", "* * * * * ?", [](auto&) { std::cout << "Hello World\n"; });
    for (;;) {
        scheduler.Tick();
        std::this_thread::sleep_for(1s);
    }

    return 0;
}
```

In order to trigger execution of callbacks one must call `oryx::chron::Scheduler::Tick` at least once a second to prevent missing schedules.

In case there is a lot of time between you call `AddSchedule` and `Tick`, you can call `RecalculateSchedule`.

`oryx::chron::Taskinformation` offers a convenient API to retrieve further information:

```cpp
#include <thread>
#include <iostream>

#include <oryx/chron.hpp>
#include "oryx/chron/task.hpp"

using namespace std::chrono_literals;

auto main() -> int {
    oryx::chron::Scheduler scheduler;

    scheduler.AddSchedule("Task-1", "* * * * * ?", [](const oryx::chron::TaskInformation& task_info) {
        if (task_info.GetDelay() >= 1s) {
            std::cout << task_info.GetName() << ": my scheduler is ticking to slow\n";
        }
    });
    for (;;) {
        scheduler.Tick();
        std::this_thread::sleep_for(2s);
    }

    return 0;
}
```

### Adding multiple tasks with individual schedules at once

Add schedule needs to sort the underlying container each time you add a schedule. To improve performance when adding a batch of tasks by only sorting once you can also call AddSchedule with:

- `std::map<std::string, std::string>`
- `std::vector<std::pair<std::string, std::string>>`
- `std::vector<std::tuple<std::string, std::string>>`
- `std::unordered_map<std::string, std::string>`

where the first element corresponds to the task name and the second element to the task schedule. Only if all schedules in the container are valid, they will be added to `oryx::chron::Scheduler`. The return type is a `std::tuple<bool, std::string, std::string>`, where the boolean is `true` if the schedules have been added or false otherwise. If the schedules have not been added, the second element in the tuple corresponds to the task-name with the given invalid schedule. If there are multiple invalid schedules in the container, `AddSchedule` will abort at the first invalid element

```cpp
#include <chrono>
#include <thread>
#include <iostream>
#include <unordered_map>

#include <oryx/chron.hpp>

using namespace std::chrono_literals;

auto main() -> int {
    oryx::chron::Scheduler scheduler;

    std::unordered_map<std::string, std::string> schedules;
    for (int i = 1; i <= 50; i++) {
        schedules["Task-" + std::to_string(i)] = "* * * * * ?";
    }

    auto res = scheduler.AddSchedule(schedules, [](const oryx::chron::TaskInformation& task_info) {
        std::cout << task_info.GetName() << ": "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(task_info.GetDelay()) << "\n";
    });

    for (;;) {
        scheduler.Tick();
        std::this_thread::sleep_for(1s);
    }

    return 0;
}
```

Adding multiple with an invalid schedule:

```cpp
std::unordered_map<std::string, std::string> schedules;
for (int i = 1; i <= 50; i++) {
    schedules["Task-" + std::to_string(i)] = "* * * * * ?";
}
schedules["Task-50"] = "invalid";

auto res = scheduler.AddSchedule(
    schedules, [](const oryx::chron::TaskInformation& task_info) { std::cout << task_info.GetName(); });
if (std::get<0>(res) == false) {
    std::cout << "Task " << std::get<1>(res) << "has an invalid schedule: " << std::get<2>(res) << "\n";
}
```



### Removing schedules from `oryx::chron::Scheduler`

`oryx::chron::Scheduler` offers two convenient functions to remove schedules:

- `ClearSchedules()` will remove all schedules
- `RemoveSchedule(std::string)` will remove a specific schedule

For example, `scheduler.RemoveSchedule("Hello from Cron")` will remove the previously added task.



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

void Ticker(MTScheduler<UTCClock>& scheduler) {
    while (!stop_requested) {
        scheduler.Tick();
        std::this_thread::sleep_for(1s);
    }
}

auto main() -> int {
    signal(SIGINT, [](int sig) { stop_requested = true; });

    MTScheduler<UTCClock> scheduler{};
    std::thread worker{Ticker, std::ref(scheduler)};
    uint64_t counter{};
    std::string task_name{};
    while (!stop_requested) {
        task_name = "Task-" + std::to_string(counter++);
        scheduler.AddSchedule(task_name, "* * * * * ?",
                              [](auto& info) { std::cout << info.GetName() << ": Called\n"; });
        std::cout << "Scheduled: " << task_name << "\n";
        std::this_thread::sleep_for(1s);
    }

    worker.join();
    std::cout << "Exiting\n";
    return 0;
}
```

## Scheduler Clock

The following clocks are available for the scheduler:

- (default) `LocalClock` offsets by system_clocks time
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

Each schedule expression conststs of 6 parts, all mandatory. However, if 'day of month' specifies specific days, then 'day of week' is ignored.

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
| @annually | Run once a year, ie.  "0 0 0 1 1 *"".
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