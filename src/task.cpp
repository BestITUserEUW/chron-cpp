#include <oryx/chron/task.hpp>

#include <format>

#include <oryx/chron/common.hpp>

using namespace std::chrono;

namespace oryx::chron {

Task::Task(std::string name, Schedule schedule, TaskFn task)
    : name_(std::move(name)),
      schedule_(std::move(schedule)),
      task_(std::move(task)),
      next_schedule_(),
      delay_(std::chrono::seconds(-1)),
      last_run_(std::numeric_limits<TimePoint>::min()),
      valid_() {}

void Task::Execute(TimePoint now) {
    // Next Schedule is still the current schedule, calculate delay (actual execution - planned execution)
    delay_ = now - next_schedule_;

    last_run_ = now;
    task_(TaskInfo(name_, delay_));
}

auto Task::CalculateNext(TimePoint from) -> bool {
    auto time_point = schedule_.CalculateFrom(from);

    // In case the calculation fails, the task will no longer expire.
    valid_ = time_point.has_value();
    if (valid_) {
        next_schedule_ = time_point.value();

        // Make sure that the task is allowed to run.
        last_run_ = next_schedule_ - 1s;
    }

    return valid_;
}

auto Task::TimeUntilExpiry(TimePoint now) const -> Duration {
    // Explicitly return 0s instead of a possibly negative duration when it has expired.
    if (now >= next_schedule_) {
        return 0s;
    }
    return next_schedule_ - now;
}

auto Task::IsExpired(TimePoint now) const -> bool { return valid_ && now >= last_run_ && TimeUntilExpiry(now) == 0s; }

auto Task::GetStatus(TimePoint now) const -> std::string {
    auto dt = Schedule::ToCalendarTime(next_schedule_);
    auto expires_in = duration_cast<milliseconds>(TimeUntilExpiry(now));
    return std::format("'{}' expires in => {}-{}-{} {}:{}:{}", name_, expires_in, dt.year, dt.month, dt.day, dt.hour,
                       dt.min, dt.sec);
}
}  // namespace oryx::chron