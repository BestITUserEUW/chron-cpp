#include "doctest.hpp"

#include <oryx/chron/scheduler.hpp>

#include <thread>
#include <chrono>
#include <format>

using namespace oryx::chron;
using namespace std::chrono;
using namespace std::chrono_literals;

namespace {

class TestClock {
public:
    using duration = system_clock::duration;
    using time_point = system_clock::time_point;

    auto Now() const -> time_point { return current_time_; }
    auto UtcOffset(time_point) const -> seconds { return 0s; }

    void SetTime(time_point t) { current_time_ = t; }
    void Advance(duration d) { current_time_ += d; }
    void Reset() {
        current_time_ = time_point{};  // epoch
    }

private:
    system_clock::time_point current_time_{};
};

auto CreateScheduleExpiringIn(system_clock::time_point now, hours h, minutes m, seconds s) -> std::string {
    now = now + h + m + s;
    auto dt = Schedule::ToCalendarTime(now);
    return std::format("{} {} {} * * ?", dt.sec, dt.min, dt.hour);
}

}  // namespace

SCENARIO("Adding a task") {
    GIVEN("A Scheduler instance with no task") {
        Scheduler c{};
        bool expired{};

        THEN("Starts with no task") { REQUIRE(c.GetNumTasks() == 0); }

        WHEN("Adding a task that runs every second") {
            REQUIRE(c.AddSchedule("A task", "* * * * * ?", [&expired](auto&) { expired = true; }));

            THEN("Count is 1 and task was not expired two seconds ago") {
                REQUIRE(c.GetNumTasks() == 1);
                c.Tick(c.GetClock().Now() - 2s);
                REQUIRE_FALSE(expired);
            }
            AND_THEN("Task is expired when calculating based on current time") {
                c.Tick();
                THEN("Task is expired") { REQUIRE(expired); }
            }
        }
    }
}

SCENARIO("Adding a task that expires in the future") {
    GIVEN("A Scheduler instance with task expiring in 3 seconds") {
        Scheduler<TestClock> c{};
        auto& clock = c.GetClock();
        bool expired{};

        REQUIRE(c.AddSchedule("A task", CreateScheduleExpiringIn(clock.Now(), 0h, 0min, 3s),
                              [&expired](auto&) { expired = true; }));

        THEN("Not yet expired") { REQUIRE_FALSE(expired); }
        AND_WHEN("When waiting one second") {
            clock.Advance(1s);
            c.Tick();
            THEN("Task has not yet expired") { REQUIRE_FALSE(expired); }
        }
        AND_WHEN("When waiting three seconds") {
            clock.Advance(3s);
            c.Tick();
            THEN("Task has expired") { REQUIRE(expired); }
        }
    }
}

SCENARIO("Get delay using Task-Information") {
    using namespace std::chrono_literals;

    GIVEN("A Scheduler instance with one task expiring in  2 seconds, but taking 3 seconds to execute") {
        auto two_second_expired = 0;
        auto delay = system_clock::duration(-1s);

        Scheduler c;

        REQUIRE(c.AddSchedule("Two", "*/2 * * * * ?", [&two_second_expired, &delay](auto& i) {
            two_second_expired++;
            delay = i.GetDelay();
            std::this_thread::sleep_for(3s);
        }));
        THEN("Not yet expired") {
            REQUIRE_FALSE(two_second_expired);
            REQUIRE(delay <= 0s);
        }
        WHEN("Exactly schedule task") {
            while (two_second_expired == 0) c.Tick();

            THEN("Task should have expired within a valid time") {
                REQUIRE(two_second_expired == 1);
                REQUIRE(delay <= 1s);
            }
            AND_THEN(
                "Executing another Tick again, leading to execute task again immediatly, but not on time as execution "
                "has taken 3 seconds.") {
                c.Tick();
                REQUIRE(two_second_expired == 2);
                REQUIRE(delay >= 1s);
            }
        }
    }
}

TEST_CASE("Task priority") {
    GIVEN("A Scheduler instance with two tasks expiring in 3 and 5 seconds, added in 'reverse' order") {
        int three_sec_expired{};
        int five_sec_expired{};

        Scheduler<TestClock> c;
        auto& clock = c.GetClock();
        REQUIRE(c.AddSchedule("Five", CreateScheduleExpiringIn(c.GetClock().Now(), 0h, 0min, 5s),
                              [&five_sec_expired](auto&) { five_sec_expired++; }));

        REQUIRE(c.AddSchedule("Three", CreateScheduleExpiringIn(c.GetClock().Now(), 0h, 0min, 3s),
                              [&three_sec_expired](auto&) { three_sec_expired++; }));

        THEN("Not yet expired") {
            REQUIRE_FALSE(three_sec_expired);
            REQUIRE_FALSE(five_sec_expired);
        }

        WHEN("Waiting 1 seconds") {
            clock.Advance(1s);
            c.Tick();

            THEN("Task has not yet expired") {
                REQUIRE(three_sec_expired == 0);
                REQUIRE(five_sec_expired == 0);
            }
        }
        AND_WHEN("Waiting 3 seconds") {
            clock.Advance(3s);
            c.Tick();

            THEN("3 second task has expired") {
                REQUIRE(three_sec_expired == 1);
                REQUIRE(five_sec_expired == 0);
            }
        }
        AND_WHEN("Waiting 5 seconds") {
            clock.Advance(5s);
            c.Tick();

            THEN("3 and 5 second task has expired") {
                REQUIRE(three_sec_expired == 1);
                REQUIRE(five_sec_expired == 1);
            }
        }
        AND_WHEN("Waiting based on the time given by the Scheduler instance") {
            clock.Advance(c.TimeUntilNext());
            c.Tick();

            THEN("3 second task has expired") {
                REQUIRE(three_sec_expired == 1);
                REQUIRE(five_sec_expired == 0);
            }
        }
        AND_WHEN("Waiting based on the time given by the Scheduler instance") {
            clock.Advance(c.TimeUntilNext());
            REQUIRE(c.Tick() == 1);

            clock.Advance(c.TimeUntilNext());
            REQUIRE(c.Tick() == 1);

            THEN("3 and 5 second task has each expired once") {
                REQUIRE(three_sec_expired == 1);
                REQUIRE(five_sec_expired == 1);
            }
        }
    }
}

SCENARIO("Clock changes") {
    GIVEN("A Scheduler instance with a single task expiring every hour") {
        Scheduler<TestClock> c{};
        auto& clock = c.GetClock();

        // Midnight
        clock.SetTime(sys_days{2018y / 05 / 05});

        // Every hour
        REQUIRE(c.AddSchedule("Clock change task", "0 0 * * * ?", [](auto&) {}));

        // https://linux.die.net/man/8/scheduler

        WHEN("Clock changes <3h forward") {
            THEN("Task expires accordingly") {
                REQUIRE(c.Tick() == 1);
                clock.Advance(minutes{30});  // 00:30
                REQUIRE(c.Tick() == 0);
                clock.Advance(minutes{30});  // 01:00
                REQUIRE(c.Tick() == 1);
                REQUIRE(c.Tick() == 0);
                REQUIRE(c.Tick() == 0);
                clock.Advance(minutes{30});  // 01:30
                REQUIRE(c.Tick() == 0);
                clock.Advance(minutes{15});  // 01:45
                REQUIRE(c.Tick() == 0);
                clock.Advance(minutes{15});  // 02:00
                REQUIRE(c.Tick() == 1);
            }
        }
        AND_WHEN("Clock is moved forward >= 3h") {
            THEN("Task are rescheduled, not run") {
                REQUIRE(c.Tick() == 1);
                clock.Advance(hours{3});     // 03:00
                REQUIRE(c.Tick() == 1);      // Rescheduled
                clock.Advance(minutes{15});  // 03:15
                REQUIRE(c.Tick() == 0);
                clock.Advance(minutes{45});  // 04:00
                REQUIRE(c.Tick() == 1);
            }
        }
        AND_WHEN("Clock is moved back <3h") {
            THEN("Tasks retain their last scheduled time and are prevented from running twice") {
                REQUIRE(c.Tick() == 1);
                clock.Advance(-hours{1});  // 23:00
                REQUIRE(c.Tick() == 0);
                clock.Advance(-hours{1});  // 22:00
                REQUIRE(c.Tick() == 0);
                clock.Advance(hours{3});  // 1:00
                REQUIRE(c.Tick() == 1);
            }
        }
        AND_WHEN("Clock is moved back >3h") {
            THEN("Tasks are rescheduled") {
                REQUIRE(c.Tick() == 1);
                clock.Advance(-hours{3});  // 21:00
                REQUIRE(c.Tick() == 1);
                REQUIRE(c.Tick() == 0);
                clock.Advance(hours{1});  // 22:00
                REQUIRE(c.Tick() == 1);
            }
        }
    }
}

SCENARIO("Multiple Ticks per second") {
    Scheduler<TestClock> c{};
    auto& clock = c.GetClock();

    auto now = sys_days{2018y / 05 / 05};
    clock.SetTime(now);

    int run_count{};

    // Every 10 seconds
    REQUIRE(c.AddSchedule("Clock change task", "*/10 0 * * * ?", [&run_count](auto&) { run_count++; }));

    c.Tick(now);

    REQUIRE(run_count == 1);

    WHEN("Many Ticks during one seconds") {
        for (auto i = 0; i < 10; ++i) {
            clock.Advance(microseconds{1});
            c.Tick();
        }

        THEN("Run Count has not increased") { REQUIRE(run_count == 1); }
    }
}

SCENARIO("Tasks can be added and removed from the scheduler") {
    GIVEN("A Scheduler instance with no task") {
        Scheduler c{};
        bool expired{};

        WHEN("Adding 5 tasks that runs every second") {
            REQUIRE(c.AddSchedule("Task-1", "* * * * * ?", [&expired](auto&) { expired = true; }));

            REQUIRE(c.AddSchedule("Task-2", "* * * * * ?", [&expired](auto&) { expired = true; }));

            REQUIRE(c.AddSchedule("Task-3", "* * * * * ?", [&expired](auto&) { expired = true; }));

            REQUIRE(c.AddSchedule("Task-4", "* * * * * ?", [&expired](auto&) { expired = true; }));

            REQUIRE(c.AddSchedule("Task-5", "* * * * * ?", [&expired](auto&) { expired = true; }));

            THEN("Count is 5") { REQUIRE(c.GetNumTasks() == 5); }
            AND_THEN("Removing all scheduled tasks") {
                c.ClearSchedules();
                REQUIRE(c.GetNumTasks() == 0);
            }
            AND_THEN("Removing a task that does not exist") {
                c.RemoveSchedule("Task-6");
                REQUIRE(c.GetNumTasks() == 5);
            }
            AND_THEN("Removing a task that does exist") {
                c.RemoveSchedule("Task-5");
                REQUIRE(c.GetNumTasks() == 4);
            }
        }
    }
}

SCENARIO("TzClock Timezone is not set fallback to utc") {
    GIVEN("No timezone") {
        TzClock tz_clock{};
        auto now = system_clock::now();
        REQUIRE(tz_clock.UtcOffset(now) == 0s);
    }
    GIVEN("A wrong timezone") {
        TzClock tz_clock{};
        auto now = system_clock::now();
        tz_clock.TrySetTimezone("404Not/Found");
        REQUIRE(tz_clock.UtcOffset(now) == 0s);
    }
}

SCENARIO("TzClock Setting time zone") {
    TzClock tz_clock{};
    GIVEN("Valid time zone") { REQUIRE(tz_clock.TrySetTimezone("Europe/Berlin")); }
    GIVEN("Invalid time zone") { REQUIRE_FALSE(tz_clock.TrySetTimezone("404Not/Found")); }
}
