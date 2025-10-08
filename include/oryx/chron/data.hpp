#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <set>
#include <string>
#include <vector>
#include <array>
#include <span>
#include <type_traits>

#include <oryx/chron/time_types.hpp>
#include <oryx/chron/details/string_split.hpp>
#include <oryx/chron/details/ctre.hpp>
#include <oryx/chron/details/string_cast.hpp>
#include <oryx/chron/details/to_underlying.hpp>
#include <oryx/chron/details/time_types_transform.hpp>

namespace oryx::chron {

enum class DataCachePolicy : uint8_t { kUseCache, kUseCacheThreadSafe, kBypassCache };

class Data {
public:
    static constexpr int kNumberOfLongMonths = 7;
    static constexpr std::array<Months, kNumberOfLongMonths> kMonthsWith31{
        Months::January, Months::March, Months::May, Months::July, Months::August, Months::October, Months::December};

    Data() = default;

    auto IsValid() const -> bool { return valid_; }
    auto GetSeconds() const -> const std::set<Seconds>& { return seconds_; }
    auto GetMinutes() const -> const std::set<Minutes>& { return minutes_; }
    auto GetHours() const -> const std::set<Hours>& { return hours_; }
    auto GetDayOfMonth() const -> const std::set<DayOfMonth>& { return day_of_month_; }
    auto GetMonths() const -> const std::set<Months>& { return months_; }
    auto GetDayOfWeek() const -> const std::set<DayOfWeek>& { return day_of_week_; }

    template <typename T>
    auto ConvertFromStringRangeToNumberRange(std::string_view range, std::set<T>& numbers) -> bool;

    template <typename T>
        requires(std::is_same_v<T, Months> || std::is_same_v<T, DayOfWeek>)
    static auto ReplaceStringNameWithNumeric(std::string& s) -> std::string&;

    template <DataCachePolicy Policy = DataCachePolicy::kUseCache>
    static auto Create(const std::string& cron_expression) -> Data;

private:
    auto Parse(std::string_view cron_expression) -> void;

    template <typename T>
    auto ValidateNumeric(std::string_view s, std::set<T>& numbers) -> bool;

    template <typename T>
    auto ValidateLiteral(const std::string& s, std::set<T>& numbers, std::span<const std::string_view> names) -> bool;

    template <typename T>
    auto ProcessParts(const std::vector<std::string>& parts, std::set<T>& numbers) -> bool;

    template <typename T>
    auto AddNumber(std::set<T>& set, int32_t number) -> bool;

    template <typename T>
    auto IsWithinLimits(int32_t low, int32_t high) -> bool;

    template <typename T>
    auto GetRange(std::string_view s, T& low, T& high) -> bool;

    template <typename T>
    auto GetStep(std::string_view s, uint8_t& start, uint8_t& step) -> bool;

    auto IsNumber(std::string_view s) -> bool;
    auto IsBetween(int32_t value, int32_t low_limit, int32_t high_limit) -> bool;
    auto ValidateDateVsMonths() const -> bool;

    template <typename T>
    auto AddFullRange(std::set<T>& set) -> void;

    std::set<Seconds> seconds_{};
    std::set<Minutes> minutes_{};
    std::set<Hours> hours_{};
    std::set<DayOfMonth> day_of_month_{};
    std::set<Months> months_{};
    std::set<DayOfWeek> day_of_week_{};
    bool valid_{};
};

template <typename T>
auto Data::ValidateNumeric(std::string_view s, std::set<T>& numbers) -> bool {
    auto parts = details::StringSplit(s, ',');
    return ProcessParts(parts, numbers);
}

template <typename T>
auto Data::ValidateLiteral(const std::string& s, std::set<T>& numbers, std::span<const std::string_view> names)
    -> bool {
    auto parts = details::StringSplit(s, ',');
    for (auto& part : parts) {
        details::ReplaceWithNumeric<T>(part, names);
    }
    return ProcessParts(parts, numbers);
}

template <typename T>
auto Data::ProcessParts(const std::vector<std::string>& parts, std::set<T>& numbers) -> bool {
    bool result{true};
    for (auto& part : parts) {
        result &= ConvertFromStringRangeToNumberRange(part, numbers);
    }
    return result;
}

template <typename T>
auto Data::GetRange(std::string_view s, T& low, T& high) -> bool {
    if (auto match = ctre::match<R"#((\d+)-(\d+))#">(s)) {
        int left = details::StringCast<int>(match.get<1>().to_view());
        int right = details::StringCast<int>(match.get<2>().to_view());
        if (IsWithinLimits<T>(left, right)) {
            low = static_cast<T>(left);
            high = static_cast<T>(right);
            return true;
        }
    }
    return false;
}

template <typename T>
auto Data::GetStep(std::string_view s, uint8_t& start, uint8_t& step) -> bool {
    if (auto match = ctre::match<R"#((\d+|\*)/(\d+))#">(s)) {
        auto first = match.get<1>().to_view();
        int raw_start = (first == "*") ? details::to_underlying(T::First) : details::StringCast<int>(first);
        int raw_step = details::StringCast<int>(match.get<2>().to_view());
        if (IsWithinLimits<T>(raw_start, raw_start) && raw_step > 0) {
            start = static_cast<uint8_t>(raw_start);
            step = static_cast<uint8_t>(raw_step);
            return true;
        }
    }
    return false;
}

template <typename T>
auto Data::AddFullRange(std::set<T>& set) -> void {
    for (auto v = details::to_underlying(T::First); v <= details::to_underlying(T::Last); ++v) {
        if (set.find(static_cast<T>(v)) == set.end()) {
            set.emplace(static_cast<T>(v));
        }
    }
}

template <typename T>
auto Data::AddNumber(std::set<T>& set, int32_t number) -> bool {
    if (set.find(static_cast<T>(number)) != set.end()) return true;
    if (!IsWithinLimits<T>(number, number)) return false;
    set.emplace(static_cast<T>(number));
    return true;
}

template <typename T>
auto Data::IsWithinLimits(int32_t low, int32_t high) -> bool {
    return IsBetween(low, details::to_underlying(T::First), details::to_underlying(T::Last)) &&
           IsBetween(high, details::to_underlying(T::First), details::to_underlying(T::Last));
}

template <typename T>
auto Data::ConvertFromStringRangeToNumberRange(std::string_view range, std::set<T>& numbers) -> bool {
    T left, right;
    uint8_t step_start, step;
    bool result{true};

    if (range == "*" || range == "?") {
        AddFullRange<T>(numbers);
    } else if (IsNumber(range)) {
        result = AddNumber<T>(numbers, details::StringCast<int32_t>(range));
    } else if (GetRange<T>(range, left, right)) {
        if (left <= right) {
            for (auto v = details::to_underlying(left); v <= details::to_underlying(right); ++v) {
                result &= AddNumber(numbers, v);
            }
        } else {
            for (auto v = details::to_underlying(left); v <= details::to_underlying(T::Last); ++v) {
                result &= AddNumber(numbers, v);
            }
            for (auto v = details::to_underlying(T::First); v <= details::to_underlying(right); ++v) {
                result &= AddNumber(numbers, v);
            }
        }
    } else if (GetStep<T>(range, step_start, step)) {
        for (auto v = step_start; v <= details::to_underlying(T::Last); v += step) {
            result &= AddNumber(numbers, v);
        }
    } else {
        result = false;
    }

    return result;
}

}  // namespace oryx::chron
