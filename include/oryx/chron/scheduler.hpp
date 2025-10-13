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
        if (!task) [[unlikely]] {
            return false;
        }

        std::lock_guard lock{tasks_mtx_};
        tasks_.emplace_back(std::move(task.value()));
        UnsafeSortTasks();
        return true;
    }

    template <typename F>
    auto AddScheduleBatch(F&& fn, std::optional<std::size_t> num_tasks = {}) -> bool {
        std::vector<Task> tasks;
        if (num_tasks) tasks.reserve(num_tasks.value());
        auto add_schedule = [this, &tasks](std::string name, std::string_view cron_expr, TaskFn work) -> bool {
            auto task = MakeTask(std::move(name), cron_expr, work);
            if (!task) [[unlikely]] {
                return false;
            }
            tasks.emplace_back(std::move(task.value()));
            return true;
        };

        std::invoke(fn, std::move(add_schedule));
        if (tasks.empty()) [[unlikely]] {
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

    void RemoveSchedule(std::string_view name) {
        std::lock_guard lock{tasks_mtx_};
        std::erase_if(tasks_, [name](const Task& t) { return t == name; });
    }

    void RecalculateSchedules() {
        std::lock_guard lock{tasks_mtx_};
        for (auto& task : tasks_) task.CalculateNext(clock_.Now() + std::chrono::seconds(1));
    }

    auto Tick(TimePoint now) -> std::size_t {
        std::lock_guard lock{tasks_mtx_};

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
            return 0;
        }

        std::size_t executed_count{};
        std::erase_if(tasks_, [&executed_count, now](Task& task) {
            if (!task.IsExpired(now)) {
                return false;
            }

            task.Execute(now);
            executed_count++;
            return !task.CalculateNext(now + std::chrono::seconds(1));
        });

        if (executed_count > 0) {
            UnsafeSortTasks();
        }

        return executed_count;
    }

    auto Tick() -> std::size_t { return Tick(clock_.Now()); }

    auto TimeUntilNext() const -> Duration {
        std::lock_guard lock{tasks_mtx_};
        if (tasks_.empty()) {
            return Duration::max();
        }
        return tasks_[0].TimeUntilExpiry(clock_.Now());
    }

    auto GetClock() -> ClockType& { return clock_; }
    auto GetParser() -> ParserType& { return parser_; }

    auto GetNumTasks() const -> std::size_t {
        std::lock_guard lock{tasks_mtx_};
        return tasks_.size();
    }

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
        if (!data) [[unlikely]] {
            return std::nullopt;
        }

        Task task(std::move(name), Schedule(std::move(data.value())), std::move(work));
        if (!task.CalculateNext(clock_.Now())) [[unlikely]] {
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
