#pragma once

#include <functional>
#include <chrono>

#include "schedule.hpp"

namespace oryx::chron {

class TaskInformation {
public:
    virtual ~TaskInformation() = default;
    virtual auto GetDelay() const -> std::chrono::system_clock::duration = 0;
    virtual auto GetName() const -> std::string_view = 0;
};

class Task : public TaskInformation {
public:
    using TaskFunction = std::function<void(const TaskInformation &)>;

    Task(std::string name, Schedule schedule, TaskFunction task);
    Task(const Task &other) = default;

    auto operator=(const Task &) -> Task & = default;
    auto operator>(const Task &other) const -> bool { return next_schedule_ > other.next_schedule_; }
    auto operator<(const Task &other) const -> bool { return next_schedule_ < other.next_schedule_; }

    void Execute(std::chrono::system_clock::time_point now);
    auto CalculateNext(std::chrono::system_clock::time_point from) -> bool;
    auto TimeUntilExpiry(std::chrono::system_clock::time_point now) const -> std::chrono::system_clock::duration;

    auto IsExpired(std::chrono::system_clock::time_point now) const -> bool;
    auto GetName() const -> std::string_view override { return name_; }
    auto GetDelay() const -> std::chrono::system_clock::duration override { return delay_; }
    auto GetStatus(std::chrono::system_clock::time_point now) const -> std::string;

private:
    std::string name_;
    Schedule schedule_;
    TaskFunction task_;
    std::chrono::system_clock::time_point next_schedule_;
    std::chrono::system_clock::duration delay_;
    std::chrono::system_clock::time_point last_run_;
    bool valid_;
};

inline auto operator==(std::string_view lhs, const Task &rhs) -> bool { return lhs == rhs.GetName(); }

inline auto operator==(const Task &lhs, std::string_view rhs) -> bool { return lhs.GetName() == rhs; }

inline auto operator!=(std::string_view lhs, const Task &rhs) -> bool { return !(lhs == rhs); }

inline auto operator!=(const Task &lhs, std::string_view rhs) -> bool { return !(lhs == rhs); }

}  // namespace oryx::chron
