#pragma once

#include <algorithm>
#include <functional>
#include <mutex>
#include <string>
#include <chrono>
#include <map>
#include <vector>

#include "traits.hpp"
#include "task.hpp"
#include "clock.hpp"

namespace oryx::chron {
namespace details {

class NullMutex {
public:
    void lock() {}
    void unlock() {}
};

}  // namespace details

template <traits::Clock ClockType = LocalClock, traits::BasicLockable LockType = details::NullMutex>
class Scheduler {
public:
    // Schedule management
    auto AddSchedule(std::string name, const std::string& schedule, Task::TaskFunction work) -> bool {
        auto data = Data::Create(schedule);
        if (!data.IsValid()) {
            return false;
        }

        Task task(std::move(name), Schedule(std::move(data)), std::move(work));
        if (!task.CalculateNext(clock_.Now())) {
            return false;
        }

        std::lock_guard lock{tasks_mtx_};
        tasks_.emplace_back(std::move(task));
        UnsafeSortTasks();
        return true;
    }

    template <typename Schedules = std::map<std::string, std::string>>
    auto AddSchedule(const Schedules& name_schedule_map, Task::TaskFunction work)
        -> std::tuple<bool, std::string, std::string> {
        bool is_valid{true};
        std::tuple<bool, std::string, std::string> result{false, "", ""};

        std::vector<Task> tasks;
        tasks.reserve(name_schedule_map.size());

        for (const auto& [name, schedule] : name_schedule_map) {
            auto data = Data::Create(schedule);
            is_valid = data.IsValid();
            if (is_valid) {
                Task task(std::move(name), Schedule(std::move(data)), work);
                if (task.CalculateNext(clock_.Now())) {
                    tasks.emplace_back(std::move(task));
                }
            } else {
                std::get<1>(result) = name;
                std::get<2>(result) = schedule;
            }
            if (!is_valid) break;
        }

        if (is_valid && !tasks.empty()) {
            std::lock_guard lock{tasks_mtx_};
            tasks_.reserve(tasks_.size() + tasks.size());
            tasks_.insert(tasks_.end(), std::make_move_iterator(tasks.begin()), std::make_move_iterator(tasks.end()));
            UnsafeSortTasks();
        }

        std::get<0>(result) = is_valid;
        return result;
    }

    void ClearSchedules() {
        std::lock_guard lock{tasks_mtx_};
        tasks_.clear();
    }

    void RemoveSchedule(const std::string& name) {
        std::lock_guard lock{tasks_mtx_};
        auto it = std::ranges::find_if(tasks_, [&name](const Task& to_compare) { return name == to_compare; });
        if (it != tasks_.end()) {
            tasks_.erase(it);
        }
    }

    void RecalculateSchedule() {
        std::lock_guard lock{tasks_mtx_};
        for (auto& task : tasks_) task.CalculateNext(clock_.Now() + std::chrono::seconds(1));
    }

    auto Tick(std::chrono::system_clock::time_point now) -> size_t {
        std::lock_guard lock{tasks_mtx_};

        size_t executed_count{};
        if (!first_tick_) [[likely]] {
            auto diff = now - last_tick_;

            if (std::chrono::abs(diff) < std::chrono::seconds{1}) {
                now = last_tick_;
            }

            if (std::chrono::abs(diff) >= std::chrono::hours{3}) {
                for (auto& task : tasks_) task.CalculateNext(now);
            }
        } else {
            first_tick_ = false;
        }

        last_tick_ = now;
        if (tasks_.empty()) {
            return executed_count;
        }

        for (auto& task : tasks_) {
            if (task.IsExpired(now)) {
                task.Execute(now);
                if (!task.CalculateNext(now + std::chrono::seconds(1))) {
                    auto it = std::ranges::find_if(tasks_, [&task](const Task& t) { return task.GetName() == t; });
                    if (it != tasks_.end()) {
                        tasks_.erase(it);
                    }
                }
                ++executed_count;
            }
        }

        if (executed_count > 0) {
            UnsafeSortTasks();
        }

        return executed_count;
    }

    auto Tick() -> size_t { return Tick(clock_.Now()); }

    auto TimeUntilNext() const -> std::chrono::system_clock::duration {
        std::lock_guard lock{tasks_mtx_};
        if (tasks_.empty()) {
            return std::chrono::minutes::max();
        }
        return tasks_[0].TimeUntilExpiry(clock_.Now());
    }

    auto GetClock() -> ClockType& { return clock_; }
    auto GetNumTasks() const -> size_t { return tasks_.size(); }

    void GetTimeUntilExpiryForTasks(
        std::vector<std::tuple<std::string, std::chrono::system_clock::duration>>& status) const {
        std::lock_guard lock{tasks_mtx_};
        auto now = clock_.Now();
        status.clear();
        status.reserve(tasks_.size());
        for (const auto& task : tasks_) {
            status.emplace_back(task.GetName(), task.TimeUntilExpiry(now));
        }
    }

private:
    void UnsafeSortTasks() { std::ranges::sort(tasks_, std::less<>{}); }

    std::vector<Task> tasks_;
    mutable std::mutex tasks_mtx_;
    ClockType clock_;
    std::chrono::system_clock::time_point last_tick_;
    bool first_tick_{true};
};

template <traits::Clock ClockType>
using MTScheduler = Scheduler<ClockType, std::mutex>;

}  // namespace oryx::chron
