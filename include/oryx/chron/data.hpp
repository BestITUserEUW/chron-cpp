#pragma once

#include <set>
#include <regex>
#include <string>
#include <vector>
#include <unordered_map>
#include <array>
#include <span>

#include "time_types.hpp"

namespace oryx::chron {

class Data {
public:
    static constexpr int kNumberOfLongMonths = 7;
    static constexpr std::array<Months, kNumberOfLongMonths> kMonthsWith31{
        Months::January, Months::March, Months::May, Months::July, Months::August, Months::October, Months::December};

    static constexpr std::array<std::string_view, 12> kMonthNames{"JAN", "FEB", "MAR", "APR", "MAY", "JUN",
                                                                  "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

    static constexpr std::array<std::string_view, 7> kDayNames{"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};

    static auto Create(const std::string& cron_expression) -> Data;

    Data() = default;
    Data(const Data&) = default;
    Data(Data&&) = default;

    auto operator=(const Data&) -> Data& = default;

    auto IsValid() const -> bool { return valid_; }
    auto GetSeconds() const -> const std::set<Seconds>& { return seconds_; }
    auto GetMinutes() const -> const std::set<Minutes>& { return minutes_; }
    auto GetHours() const -> const std::set<Hours>& { return hours_; }
    auto GetDayOfMonth() const -> const std::set<DayOfMonth>& { return day_of_month_; }
    auto GetMonths() const -> const std::set<Months>& { return months_; }
    auto GetDayOfWeek() const -> const std::set<DayOfWeek>& { return day_of_week_; }

    // Template helpers
    template <typename T>
    static auto ValueOf(T t) -> uint8_t {
        return static_cast<uint8_t>(t);
    }

    template <typename T>
    static auto HasAnyInRange(const std::set<T>& set, uint8_t low, uint8_t high) -> bool {
        for (uint8_t i = low; i <= high; ++i) {
            if (set.find(static_cast<T>(i)) != set.end()) {
                return true;
            }
        }
        return false;
    }

    template <typename T>
    auto ConvertFromStringRangeToNumberRange(const std::string& range, std::set<T>& numbers) -> bool;

    template <typename T>
    static auto ReplaceStringNameWithNumeric(std::string& s) -> std::string&;

private:
    auto Parse(const std::string& cron_expression) -> void;

    template <typename T>
    auto ValidateNumeric(const std::string& s, std::set<T>& numbers) -> bool;

    template <typename T>
    auto ValidateLiteral(const std::string& s, std::set<T>& numbers, std::span<const std::string_view> names) -> bool;

    template <typename T>
    auto ProcessParts(const std::vector<std::string>& parts, std::set<T>& numbers) -> bool;

    template <typename T>
    auto AddNumber(std::set<T>& set, int32_t number) -> bool;

    template <typename T>
    auto IsWithinLimits(int32_t low, int32_t high) -> bool;

    template <typename T>
    auto GetRange(const std::string& s, T& low, T& high) -> bool;

    template <typename T>
    auto GetStep(const std::string& s, uint8_t& start, uint8_t& step) -> bool;

    auto Split(const std::string& s, char token) -> std::vector<std::string>;
    auto IsNumber(const std::string& s) -> bool;
    auto IsBetween(int32_t value, int32_t low_limit, int32_t high_limit) -> bool;
    auto ValidateDateVsMonths() const -> bool;
    auto CheckDomVsDow(const std::string& dom, const std::string& dow) const -> bool;

    template <typename T>
    auto AddFullRange(std::set<T>& set) -> void;

    std::set<Seconds> seconds_;
    std::set<Minutes> minutes_;
    std::set<Hours> hours_;
    std::set<DayOfMonth> day_of_month_;
    std::set<Months> months_;
    std::set<DayOfWeek> day_of_week_;
    bool valid_ = false;

    static std::unordered_map<std::string, Data> cache_;
};

template <typename T>
auto Data::ValidateNumeric(const std::string& s, std::set<T>& numbers) -> bool {
    auto parts = Split(s, ',');
    return ProcessParts(parts, numbers);
}

template <typename T>
auto Data::ValidateLiteral(const std::string& s, std::set<T>& numbers, std::span<const std::string_view> names)
    -> bool {
    auto parts = Split(s, ',');
    auto value_of_first_name = ValueOf(T::First);

    for (const auto& name : names) {
        std::regex regex_pattern(name.data(), std::regex_constants::ECMAScript | std::regex_constants::icase);

        for (auto& part : parts) {
            std::string replaced;
            std::regex_replace(std::back_inserter(replaced), part.begin(), part.end(), regex_pattern,
                               std::to_string(value_of_first_name));
            part = replaced;
        }
        ++value_of_first_name;
    }

    return ProcessParts(parts, numbers);
}

template <typename T>
auto Data::ProcessParts(const std::vector<std::string>& parts, std::set<T>& numbers) -> bool {
    bool result = true;
    for (const auto& part : parts) {
        result &= ConvertFromStringRangeToNumberRange(part, numbers);
    }
    return result;
}

template <typename T>
auto Data::GetRange(const std::string& s, T& low, T& high) -> bool {
    std::regex range_regex(R"#((\d+)-(\d+))#");
    std::smatch match;
    if (std::regex_match(s.begin(), s.end(), match, range_regex)) {
        int left = std::stoi(match[1].str());
        int right = std::stoi(match[2].str());
        if (IsWithinLimits<T>(left, right)) {
            low = static_cast<T>(left);
            high = static_cast<T>(right);
            return true;
        }
    }
    return false;
}

template <typename T>
auto Data::GetStep(const std::string& s, uint8_t& start, uint8_t& step) -> bool {
    std::regex step_regex(R"#((\d+|\*)/(\d+))#");
    std::smatch match;
    if (std::regex_match(s.begin(), s.end(), match, step_regex)) {
        int raw_start = (match[1].str() == "*") ? ValueOf(T::First) : std::stoi(match[1].str());
        int raw_step = std::stoi(match[2].str());
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
    for (auto v = ValueOf(T::First); v <= ValueOf(T::Last); ++v) {
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
    return IsBetween(low, ValueOf(T::First), ValueOf(T::Last)) && IsBetween(high, ValueOf(T::First), ValueOf(T::Last));
}

template <typename T>
auto Data::ConvertFromStringRangeToNumberRange(const std::string& range, std::set<T>& numbers) -> bool {
    T left, right;
    uint8_t step_start, step;
    bool result = true;

    if (range == "*" || range == "?") {
        AddFullRange<T>(numbers);
    } else if (IsNumber(range)) {
        result = AddNumber<T>(numbers, std::stoi(range));
    } else if (GetRange<T>(range, left, right)) {
        if (left <= right) {
            for (auto v = ValueOf(left); v <= ValueOf(right); ++v) {
                result &= AddNumber(numbers, v);
            }
        } else {
            for (auto v = ValueOf(left); v <= ValueOf(T::Last); ++v) {
                result &= AddNumber(numbers, v);
            }
            for (auto v = ValueOf(T::First); v <= ValueOf(right); ++v) {
                result &= AddNumber(numbers, v);
            }
        }
    } else if (GetStep<T>(range, step_start, step)) {
        for (auto v = step_start; v <= ValueOf(T::Last); v += step) {
            result &= AddNumber(numbers, v);
        }
    } else {
        result = false;
    }

    return result;
}

template <typename T>
auto Data::ReplaceStringNameWithNumeric(std::string& s) -> std::string& {
    auto value = ValueOf(T::First);
    static_assert(std::is_same<T, Months>() || std::is_same<T, DayOfWeek>(), "T must be either Months or DayOfWeek");

    if constexpr (std::is_same<T, Months>()) {
        for (auto name : kMonthNames) {
            std::regex regex_pattern(name.data(), std::regex_constants::ECMAScript | std::regex_constants::icase);
            std::string replaced;
            std::regex_replace(std::back_inserter(replaced), s.begin(), s.end(), regex_pattern, std::to_string(value));
            s = replaced;
            ++value;
        }
    } else {
        for (auto name : kDayNames) {
            std::regex regex_pattern(name.data(), std::regex_constants::ECMAScript | std::regex_constants::icase);
            std::string replaced;
            std::regex_replace(std::back_inserter(replaced), s.begin(), s.end(), regex_pattern, std::to_string(value));
            s = replaced;
            ++value;
        }
    }
    return s;
}
}  // namespace oryx::chron
