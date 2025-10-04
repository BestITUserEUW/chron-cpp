#pragma once

#include <functional>
#include <string>

#include "schedule.hpp"
#include "chrono_types.h"

namespace oryx::chron {

class TaskInformation {
public:
    virtual ~TaskInformation() = default;
    virtual auto GetDelay() const -> Duration = 0;
    virtual auto GetName() const -> std::string_view = 0;
};

class Task : public TaskInformation {
public:
    using TaskFn = std::function<void(const TaskInformation &)>;

    Task(std::string name, Schedule schedule, TaskFn task);

    auto operator=(const Task &) -> Task & = default;
    auto operator>(const Task &other) const -> bool { return next_schedule_ > other.next_schedule_; }
    auto operator<(const Task &other) const -> bool { return next_schedule_ < other.next_schedule_; }

    void Execute(TimePoint now);
    auto CalculateNext(TimePoint from) -> bool;
    auto TimeUntilExpiry(TimePoint now) const -> Duration;

    auto IsExpired(TimePoint now) const -> bool;
    auto GetName() const -> std::string_view override { return name_; }
    auto GetDelay() const -> Duration override { return delay_; }
    auto GetStatus(TimePoint now) const -> std::string;

private:
    std::string name_;
    Schedule schedule_;
    TaskFn task_;
    TimePoint next_schedule_;
    Duration delay_;
    TimePoint last_run_;
    bool valid_;
};

inline auto operator==(std::string_view lhs, const Task &rhs) -> bool { return lhs == rhs.GetName(); }

inline auto operator==(const Task &lhs, std::string_view rhs) -> bool { return lhs.GetName() == rhs; }

inline auto operator!=(std::string_view lhs, const Task &rhs) -> bool { return !(lhs == rhs); }

inline auto operator!=(const Task &lhs, std::string_view rhs) -> bool { return !(lhs == rhs); }

}  // namespace oryx::chron
