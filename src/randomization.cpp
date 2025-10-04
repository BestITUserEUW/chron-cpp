#include <oryx/chron/randomization.hpp>

#include <algorithm>
#include <regex>
#include <map>
#include <array>
#include <algorithm>
#include <iterator>

#include <oryx/chron/time_types.hpp>
#include <oryx/chron/data.hpp>

namespace oryx::chron {
namespace {

const std::regex kRandomExpr{R"#([rR]\((\d+)\-(\d+)\))#", std::regex_constants::ECMAScript};

auto ToView(const std::smatch& match, size_t idx) -> std::string_view {
    return {&*match[idx].first, static_cast<size_t>(match[idx].length())};
}

template <typename T>
auto GetRandomInRange(std::string_view section,
                      int& selected_value,
                      std::mt19937& twister,
                      std::pair<int, int> limit = std::make_pair(-1, -1)) -> std::pair<bool, std::string> {
    auto result = std::make_pair(true, std::string{});
    selected_value = -1;

    std::cmatch random_match;

    if (std::regex_match(section.cbegin(), section.cend(), random_match, kRandomExpr)) {
        // Random range, parse left and right numbers
        auto left = std::stoi(random_match[1].str());
        auto right = std::stoi(random_match[2].str());

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
                if (Data::ValueOf(*it) < limit.first || Data::ValueOf(*it) > limit.second) {
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
            selected_value = Data::ValueOf(*it);
            result.second = std::to_string(selected_value);
        }
    } else {
        // Not a random section, return as-is
        result.second = section;
    }

    return result;
}

auto DayLimiter(const std::set<Months>& months) -> std::pair<int, int> {
    int max = Data::ValueOf(DayOfMonth::Last);

    for (auto month : months) {
        if (month == Months::February) {
            // Limit to 29 days, possibly causing delaying schedule until next leap year.
            max = std::min(max, 29);
        } else if (std::ranges::find(Data::kMonthsWith31, month) == std::end(Data::kMonthsWith31)) {
            // Not among the months with 31 days
            max = std::min(max, 30);
        }
    }

    return {Data::ValueOf(DayOfMonth::First), max};
}

}  // namespace

Randomization::Randomization()
    : twister_(random_device_()) {}

auto Randomization::Parse(const std::string& cron_schedule) -> std::tuple<bool, std::string> {
    // Split on space to get each separate part, six parts expected
    const std::regex kSplit{R"#(^\s*(.*?)\s+(.*?)\s+(.*?)\s+(.*?)\s+(.*?)\s+(.*?)\s*$)#",
                            std::regex_constants::ECMAScript};

    std::smatch all_sections;
    auto res = std::regex_match(cron_schedule.cbegin(), cron_schedule.cend(), all_sections, kSplit);

    // Replace text with numbers
    std::string working_copy{};

    if (res) {
        // Replace month and day names first
        auto month = all_sections[5].str();
        Data::ReplaceStringNameWithNumeric<Months>(month);

        auto dow = all_sections[6].str();
        Data::ReplaceStringNameWithNumeric<DayOfWeek>(dow);

        // Merge all sections into one string
        working_copy = std::format("{} {} {} {} {} {}", ToView(all_sections, 1), ToView(all_sections, 2),
                                   ToView(all_sections, 3), ToView(all_sections, 4), month, dow);
    }

    std::string final_cron_schedule{};

    // Split again on space
    res = res && std::regex_match(working_copy.cbegin(), working_copy.cend(), all_sections, kSplit);

    if (res) {
        int selected_value = -1;
        auto second = GetRandomInRange<Seconds>(ToView(all_sections, 1), selected_value, twister_);
        res = second.first;
        final_cron_schedule = second.second;

        auto minute = GetRandomInRange<Minutes>(ToView(all_sections, 2), selected_value, twister_);
        res &= minute.first;
        final_cron_schedule += " " + minute.second;

        auto hour = GetRandomInRange<Hours>(ToView(all_sections, 3), selected_value, twister_);
        res &= hour.first;
        final_cron_schedule += " " + hour.second;

        // Do Month before DayOfMonth to allow capping the allowed range.
        auto month = GetRandomInRange<Months>(ToView(all_sections, 5), selected_value, twister_);
        res &= month.first;

        std::set<Months> month_range{};

        if (selected_value == -1) {
            // Month is not specific, get the range.
            Data cr;
            res &= cr.ConvertFromStringRangeToNumberRange<Months>(all_sections[5].str(), month_range);
        } else {
            month_range.emplace(static_cast<Months>(selected_value));
        }

        auto limits = DayLimiter(month_range);

        auto day_of_month = GetRandomInRange<DayOfMonth>(ToView(all_sections, 4), selected_value, twister_, limits);

        res &= day_of_month.first;
        final_cron_schedule += " " + day_of_month.second + " " + month.second;

        auto day_of_week = GetRandomInRange<DayOfWeek>(ToView(all_sections, 6), selected_value, twister_);
        res &= day_of_week.first;
        final_cron_schedule += " " + day_of_week.second;
    }

    return {res, final_cron_schedule};
}

}  // namespace oryx::chron
