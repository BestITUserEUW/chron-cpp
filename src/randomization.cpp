#include <oryx/chron/randomization.hpp>

#include <algorithm>
#include <regex>
#include <map>
#include <array>
#include <iterator>
#include <format>

#include <oryx/chron/time_types.hpp>
#include <oryx/chron/data.hpp>
#include <oryx/chron/details/ctre.hpp>
#include <oryx/chron/details/string_cast.hpp>
#include <oryx/chron/details/to_underlying.hpp>

namespace oryx::chron {
namespace {

template <typename T>
auto GetRandomInRange(std::string_view section,
                      int& selected_value,
                      std::mt19937& twister,
                      std::pair<int, int> limit = std::make_pair(-1, -1)) -> std::pair<bool, std::string> {
    auto result = std::make_pair(true, std::string{});
    selected_value = -1;

    if (auto match = ctre::match<R"#([rR]\((\d+)\-(\d+)\))#">(section)) {
        // Random range, parse left and right numbers
        auto left = details::StringCast<int>(match.get<1>().to_view());
        auto right = details::StringCast<int>(match.get<2>().to_view());

        // Apply limit if provided
        if (limit.first != -1 && limit.second != -1) {
            left = std::clamp(left, limit.first, limit.second);
            right = std::clamp(right, limit.first, limit.second);
        }

        Data cron_data;
        std::set<T> numbers;
        result.first = cron_data.ConvertFromStringRangeToNumberRange<T>(std::format("{}-{}", left, right), numbers);

        // Remove items outside the limit
        if (limit.first != -1 && limit.second != -1) {
            for (auto it = numbers.begin(); it != numbers.end();) {
                if (details::to_underlying(*it) < limit.first || details::to_underlying(*it) > limit.second) {
                    it = numbers.erase(it);
                } else {
                    ++it;
                }
            }
        }

        if (result.first && !numbers.empty()) {
            // Select a random value from the valid numbers
            std::uniform_int_distribution<> distribution(0, static_cast<int>(numbers.size() - 1));
            auto it = numbers.begin();
            std::advance(it, distribution(twister));
            selected_value = details::to_underlying(*it);
            result.second = std::to_string(selected_value);
        }
    } else {
        // Not a random section, return as-is
        result.second = section;
    }

    return result;
}

auto DayLimiter(const std::set<Months>& months) -> std::pair<int, int> {
    int max = details::to_underlying(DayOfMonth::Last);

    for (auto month : months) {
        if (month == Months::February) {
            // Limit to 29 days, possibly causing delaying schedule until next leap year.
            max = std::min(max, 29);
        } else if (std::ranges::find(Data::kMonthsWith31, month) == std::end(Data::kMonthsWith31)) {
            // Not among the months with 31 days
            max = std::min(max, 30);
        }
    }

    return {details::to_underlying(DayOfMonth::First), max};
}

}  // namespace

Randomization::Randomization()
    : twister_(random_device_()) {}

auto Randomization::Parse(std::string_view cron_schedule) -> std::tuple<bool, std::string> {
    // Split on space to get each separate part, six parts expected
    auto matcher = ctre::match<R"#(^\s*(.*?)\s+(.*?)\s+(.*?)\s+(.*?)\s+(.*?)\s+(.*?)\s*$)#">;
    // Replace text with numbers
    std::string working_copy{};

    if (auto match = matcher(cron_schedule)) {
        // Replace month and day names first
        auto month = match.get<5>().to_string();
        Data::ReplaceStringNameWithNumeric<Months>(month);

        auto dow = match.get<6>().to_string();
        Data::ReplaceStringNameWithNumeric<DayOfWeek>(dow);

        // Merge all sections into one string
        working_copy = std::format("{} {} {} {} {} {}", match.get<1>().to_view(), match.get<2>().to_view(),
                                   match.get<3>().to_view(), match.get<4>().to_view(), month, dow);
    }

    std::string final_cron_schedule{};
    bool success{};
    if (auto match = matcher(working_copy)) {
        int selected_value = -1;
        auto second = GetRandomInRange<Seconds>(match.get<1>().to_view(), selected_value, twister_);
        success = second.first;
        final_cron_schedule = second.second;

        auto minute = GetRandomInRange<Minutes>(match.get<2>().to_view(), selected_value, twister_);
        success &= minute.first;
        final_cron_schedule += " " + minute.second;

        auto hour = GetRandomInRange<Hours>(match.get<3>().to_view(), selected_value, twister_);
        success &= hour.first;
        final_cron_schedule += " " + hour.second;

        // Do Month before DayOfMonth to allow capping the allowed range.
        auto month = GetRandomInRange<Months>(match.get<5>().to_view(), selected_value, twister_);
        success &= month.first;

        std::set<Months> month_range{};
        if (selected_value == -1) {
            // Month is not specific, get the range.
            Data cr;
            success &= cr.ConvertFromStringRangeToNumberRange<Months>(match.get<5>().to_string(), month_range);
        } else {
            month_range.emplace(static_cast<Months>(selected_value));
        }

        auto limits = DayLimiter(month_range);

        auto day_of_month = GetRandomInRange<DayOfMonth>(match.get<4>().to_view(), selected_value, twister_, limits);

        success &= day_of_month.first;
        final_cron_schedule += " " + day_of_month.second + " " + month.second;

        auto day_of_week = GetRandomInRange<DayOfWeek>(match.get<6>().to_view(), selected_value, twister_);
        success &= day_of_week.first;
        final_cron_schedule += " " + day_of_week.second;
    }

    return {success, final_cron_schedule};
}

}  // namespace oryx::chron
