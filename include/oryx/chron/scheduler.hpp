#pragma once

#include <algorithm>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <chrono>
#include <vector>

#include <oryx/chron/traits.hpp>
#include <oryx/chron/task.hpp>
#include <oryx/chron/clock.hpp>
#include <oryx/chron/chrono_types.hpp>
#include <oryx/chron/parser.hpp>

namespace oryx::chron {

template <traits::Clock ClockType = LocalClock,
          traits::BasicLockable MutexType = details::NullMutex,
          traits::Parser ParserType = ExpressionParser>
class Scheduler {
public:
    Scheduler() = default;

    auto AddSchedule(std::string name, std::string_view cron_expr, TaskFn work) -> bool {
        auto task = MakeTask(std::move(name), cron_expr, std::move(work));
        if (!task) {
            return false;
        }

        std::lock_guard lock{tasks_mtx_};
        tasks_.emplace_back(std::move(task.value()));
        UnsafeSortTasks();
        return true;
    }

    template <typename F>
    auto AddScheduleBatch(F&& fn, std::optional<size_t> num_tasks = std::nullopt) -> bool {
        std::vector<Task> tasks;
        if (num_tasks) tasks.reserve(num_tasks.value());
        auto add_schedule = [this, &tasks](std::string name, std::string_view cron_expr, TaskFn work) -> bool {
            auto task = MakeTask(std::move(name), cron_expr, work);
            if (!task) return false;
            tasks.emplace_back(std::move(task.value()));
            return true;
        };

        std::invoke(fn, std::move(add_schedule));
        if (tasks.empty()) {
            return false;
        }

        std::lock_guard lock{tasks_mtx_};
        tasks_.reserve(tasks_.size() + tasks.size());
        tasks_.insert(tasks_.end(), std::make_move_iterator(tasks.begin()), std::make_move_iterator(tasks.end()));
        UnsafeSortTasks();
        return true;
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

    void RecalculateSchedules() {
        std::lock_guard lock{tasks_mtx_};
        for (auto& task : tasks_) task.CalculateNext(clock_.Now() + std::chrono::seconds(1));
    }

    auto Tick(TimePoint now) -> size_t {
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

    auto TimeUntilNext() const -> Duration {
        std::lock_guard lock{tasks_mtx_};
        if (tasks_.empty()) {
            return std::chrono::minutes::max();
        }
        return tasks_[0].TimeUntilExpiry(clock_.Now());
    }

    auto GetClock() -> ClockType& { return clock_; }
    auto GetParser() -> ParserType& { return parser_; }
    auto GetNumTasks() const -> size_t { return tasks_.size(); }

    auto GetTasksStatus() const -> std::vector<std::string> {
        std::vector<std::string> status{};
        auto now = clock_.Now();

        std::lock_guard lock{tasks_mtx_};
        status.reserve(tasks_.size());
        for (const auto& task : tasks_) {
            status.emplace_back(task.GetStatus(now));
        }
        return status;
    }

private:
    auto MakeTask(std::string name, std::string_view cron_expr, TaskFn work) const -> std::optional<Task> {
        auto data = parser_(cron_expr);
        if (!data) {
            return std::nullopt;
        }

        Task task(std::move(name), Schedule(std::move(data.value())), std::move(work));
        if (!task.CalculateNext(clock_.Now())) {
            return std::nullopt;
        }
        return task;
    }

    void UnsafeSortTasks() { std::ranges::sort(tasks_, std::less<>{}); }

    std::vector<Task> tasks_{};
    mutable MutexType tasks_mtx_{};
    ClockType clock_{};
    ParserType parser_{};
    TimePoint last_tick_{};
    bool first_tick_{true};
};

template <traits::Clock ClockType = LocalClock>
using CScheduler = Scheduler<ClockType, details::NullMutex, CachedExpressionParser<details::NullMutex>>;

template <traits::Clock ClockType = LocalClock>
using MTScheduler = Scheduler<ClockType, std::mutex>;

template <traits::Clock ClockType = LocalClock>
using MTCScheduler = Scheduler<ClockType, std::mutex, CachedExpressionParser<std::mutex>>;

}  // namespace oryx::chron
