#include <oryx/chron/randomization.hpp>

#include <optional>
#include <algorithm>
#include <array>
#include <iterator>
#include <format>

#include <oryx/chron/time_types.hpp>
#include <oryx/chron/preprocessor.hpp>
#include <oryx/chron/details/ctre.hpp>
#include <oryx/chron/details/string_cast.hpp>
#include <oryx/chron/details/to_underlying.hpp>
#include <oryx/chron/details/parser.hpp>
#include <oryx/chron/details/in_range.hpp>

namespace oryx::chron {
namespace {

constexpr int kSelectedValueNone = -1;

template <typename T>
auto GetRandomInRange(std::string_view section,
                      int& selected_value,
                      std::mt19937& twister,
                      std::pair<int, int> limit = std::make_pair(-1, -1)) -> std::optional<std::string> {
    selected_value = kSelectedValueNone;

    auto match = ctre::match<R"#([rR]\((\d+)\-(\d+)\))#">(section);
    if (!match) {
        return std::string(section);
    }

    // Random range, parse left and right numbers
    auto left = details::StringCast<int>(match.get<1>().to_view());
    auto right = details::StringCast<int>(match.get<2>().to_view());

    // Apply limit if provided
    if (limit.first != -1 && limit.second != -1) {
        left = std::clamp(left, limit.first, limit.second);
        right = std::clamp(right, limit.first, limit.second);
    }

    std::set<T> numbers;
    bool success = details::Parser::ConvertFromStringRangeToNumberRange(std::format("{}-{}", left, right), numbers);

    // Remove items outside the limit
    if (limit.first != -1 && limit.second != -1) {
        std::erase_if(numbers, [&limit](T val) -> bool {
            return !details::InRange<int>(details::to_underlying(val), limit.first, limit.second);
        });
    }

    if (!success || numbers.empty()) {
        return std::nullopt;
    }

    std::uniform_int_distribution distribution(0, static_cast<int>(numbers.size() - 1));
    auto it = numbers.begin();
    std::advance(it, distribution(twister));
    selected_value = details::to_underlying(*it);
    return std::to_string(selected_value);
}

auto DayLimiter(const std::set<Months>& months) -> std::pair<int, int> {
    int max = details::to_underlying(MonthDays::Last);

    for (auto month : months) {
        if (month == Months::February) {
            // Limit to 29 days, possibly causing delaying schedule until next leap year.
            max = std::min(max, 29);
        } else if (std::ranges::find(kMonthsWith31, month) == kMonthsWith31.end()) {
            // Not among the months with 31 days
            max = std::min(max, 30);
        }
    }

    return {details::to_underlying(MonthDays::First), max};
}

}  // namespace

Randomization::Randomization()
    : twister_(random_device_()) {}

auto Randomization::Parse(std::string_view cron_schedule) -> std::optional<std::string> {
    static constexpr auto matcher = ctre::match<R"#(^\s*(.*?)\s+(.*?)\s+(.*?)\s+(.*?)\s+(.*?)\s+(.*?)\s*$)#">;

    auto preprocessed = PreprocessExpression<WeekMonthDayLiteralProcessor>(std::string(cron_schedule));
    auto match = matcher(std::move(preprocessed));
    if (!match) [[unlikely]] {
        return std::nullopt;
    }

    int selected_value = kSelectedValueNone;
    auto second = GetRandomInRange<Seconds>(match.get<1>().to_view(), selected_value, twister_);
    if (!second) [[unlikely]] {
        return std::nullopt;
    }

    auto minute = GetRandomInRange<Minutes>(match.get<2>().to_view(), selected_value, twister_);
    if (!minute) [[unlikely]] {
        return std::nullopt;
    }

    auto hour = GetRandomInRange<Hours>(match.get<3>().to_view(), selected_value, twister_);
    if (!hour) [[unlikely]] {
        return std::nullopt;
    }

    // Do Month before MonthDays to allow capping the allowed range.
    auto month = GetRandomInRange<Months>(match.get<5>().to_view(), selected_value, twister_);
    if (!month) [[unlikely]] {
        return std::nullopt;
    }

    std::set<Months> month_range{};
    if (selected_value == kSelectedValueNone) {
        // Month is not specific, get the range.
        if (!details::Parser::ConvertFromStringRangeToNumberRange(match.get<5>().to_view(), month_range)) [[unlikely]] {
            return std::nullopt;
        }
    } else {
        month_range.emplace(static_cast<Months>(selected_value));
    }

    auto limits = DayLimiter(month_range);
    auto day_of_month = GetRandomInRange<MonthDays>(match.get<4>().to_view(), selected_value, twister_, limits);
    if (!day_of_month) [[unlikely]] {
        return std::nullopt;
    }

    auto day_of_week = GetRandomInRange<Weekdays>(match.get<6>().to_view(), selected_value, twister_);
    if (!day_of_week) [[unlikely]] {
        return std::nullopt;
    }

    return std::format("{} {} {} {} {} {}", *second, *minute, *hour, *day_of_month, *month, *day_of_week);
}

}  // namespace oryx::chron
