#pragma once

#include <set>
#include <algorithm>
#include <ranges>
#include <type_traits>

#include <oryx/chron/traits.hpp>
#include <oryx/chron/chron_data.hpp>
#include <oryx/chron/time_types.hpp>

#include "string_split.hpp"
#include "time_types.hpp"
#include "string_cast.hpp"
#include "in_range.hpp"
#include "to_underlying.hpp"
#include "any_of.hpp"
#include "ctre.hpp"

namespace oryx::chron::details {

struct Parser {
    static auto IsNumber(const std::string_view sv) -> bool {
        return !sv.empty() && std::ranges::all_of(sv, ::isdigit);
    }

    template <chron::traits::TimeType T>
    static auto IsWithinBounds(int low, int high) -> bool {
        constexpr auto first = details::to_underlying(T::First);
        constexpr auto last = details::to_underlying(T::Last);
        return details::InRange<int>(low, first, last) && details::InRange<int>(high, first, last);
    }

    template <chron::traits::TimeType T>
    static auto AddNumber(std::set<T>& set, int number) -> bool {
        if (set.find(static_cast<T>(number)) != set.end()) {
            return true;
        }

        if (!IsWithinBounds<T>(number, number)) {
            return false;
        }

        set.emplace(static_cast<T>(number));
        return true;
    }

    template <chron::traits::TimeType T>
    static auto GetRange(std::string_view sv, T& low, T& high) -> bool {
        if (auto match = ctre::match<R"#((\d+)-(\d+))#">(sv)) {
            int lhs = details::StringCast<int>(match.get<1>().to_view());
            int rhs = details::StringCast<int>(match.get<2>().to_view());
            if (IsWithinBounds<T>(lhs, rhs)) {
                low = static_cast<T>(lhs);
                high = static_cast<T>(rhs);
                return true;
            }
        }
        return false;
    }

    template <chron::traits::TimeType T>
    static auto GetStep(std::string_view s, uint8_t& start, uint8_t& step) -> bool {
        if (auto match = ctre::match<R"#((\d+|\*)/(\d+))#">(s)) {
            auto first = match.get<1>().to_view();
            int raw_start = (first == "*") ? details::to_underlying(T::First) : details::StringCast<int>(first);
            int raw_step = details::StringCast<int>(match.get<2>().to_view());
            if (IsWithinBounds<T>(raw_start, raw_start) && raw_step > 0) {
                start = static_cast<uint8_t>(raw_start);
                step = static_cast<uint8_t>(raw_step);
                return true;
            }
        }
        return false;
    }

    template <chron::traits::TimeType T>
    static void AddFullRange(std::set<T>& set) {
        for (auto v = details::to_underlying(T::First); v <= details::to_underlying(T::Last); ++v) {
            if (set.find(static_cast<T>(v)) == set.end()) {
                set.emplace(static_cast<T>(v));
            }
        }
    }

    template <chron::traits::TimeType T>
    static auto AddWrappingRange(std::set<T>& numbers, T left, T right) -> bool {
        bool success{true};
        constexpr auto last = details::to_underlying(T::Last);
        constexpr auto first = details::to_underlying(T::First);

        for (auto value = details::to_underlying(left); value <= last; ++value) {
            success &= AddNumber(numbers, value);
        }
        for (auto value = first; value <= details::to_underlying(right); ++value) {
            success &= AddNumber(numbers, value);
        }

        return success;
    }

    template <chron::traits::TimeType T>
    static auto AddRange(std::set<T>& numbers, T left, T right) -> bool {
        bool success{true};

        if (left <= right) {
            for (auto value = details::to_underlying(left); value <= details::to_underlying(right); ++value) {
                success &= AddNumber(numbers, value);
            }
        } else {
            AddWrappingRange(numbers, left, right);
        }

        return success;
    }

    template <chron::traits::TimeType T>
    static auto AddStepRange(std::set<T>& numbers, uint8_t step_start, uint8_t step) -> bool {
        bool success = true;
        const auto last_value = details::to_underlying(T::Last);

        for (auto value = step_start; value <= last_value; value += step) {
            success &= AddNumber(numbers, value);
        }

        return success;
    }

    template <chron::traits::TimeType T>
    static auto ConvertFromStringRangeToNumberRange(std::string_view range, std::set<T>& numbers) -> bool {
        if (range == "*" || range == "?") {
            AddFullRange(numbers);
            return true;
        }

        if (IsNumber(range)) {
            return AddNumber(numbers, details::StringCast<int>(range));
        }

        T left, right;
        if (GetRange(range, left, right)) {
            return AddRange(numbers, left, right);
        }

        uint8_t step_start, step;
        if (GetStep<T>(range, step_start, step)) {
            return AddStepRange(numbers, step_start, step);
        }

        return false;  // Invalid format
    }

    template <chron::traits::TimeType T>
    static auto ProcessParts(auto&& parts, std::set<T>& numbers) -> bool {
        return std::ranges::all_of(parts, [&numbers](auto&& part) {
            if constexpr (std::is_convertible_v<decltype(part), std::string_view>)
                return ConvertFromStringRangeToNumberRange<T>(part, numbers);
            else
                return ConvertFromStringRangeToNumberRange(std::string_view(&*part.begin(), part.size()), numbers);
        });
    }

    template <chron::traits::TimeType T>
    static auto ValidateNumeric(std::string_view s, std::set<T>& numbers) -> bool {
        return ProcessParts(std::views::split(s, ','), numbers);
    }

    template <chron::traits::TimeType T>
    static auto ValidateLiteral(const std::string& s, std::set<T>& numbers, std::span<const std::string_view> names)
        -> bool {
        auto parts = details::StringSplit(s, ',');
        std::ranges::for_each(parts, [&names](auto&& part) { details::ReplaceWithNumeric<T>(part, names); });
        return ProcessParts(parts, numbers);
    }

    static auto CheckDomVsDow(std::string_view dom, std::string_view dow) -> bool {
        // Day of month and day of week are mutually exclusive so one of them must at always be ignored using
        // the '?'-character unless one field already is something other than '*'.
        //
        // Since we treat an ignored field as allowing the full range, we're OK with both being flagged
        // as ignored. To make it explicit to the user of the library, we do however require the use of
        // '?' as the ignore flag, although it is functionally equivalent to '*'.

        auto check = [](std::string_view l, std::string_view r) { return l == "*" && (r != "*" || r == "?"); };
        return (dom == "?" || dow == "?") || check(dom, dow) || check(dow, dom);
    }

    static auto ValidateDateVsMonths(const ChronData& data) -> bool {
        // Only February allowed? Ensure day_of_month includes only 1..29
        if (data.months.size() == 1 && data.months.contains(static_cast<Months>(2))) {
            if (!details::AnyOf(data.days, 1, 29)) return false;
        }

        // If only day 31 is selected, ensure at least one month allows it
        if (data.days.size() == 1 && data.days.contains(MonthDays::Last)) {
            if (!std::ranges::any_of(details::kMonthsWith31, [&data](Months m) { return data.months.contains(m); })) {
                return false;
            }
        }

        return true;
    }
};

}  // namespace oryx::chron::details