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

Randomization::Randomization()
    : twister_(random_device_()) {}

auto Randomization::Parse(const std::string& cron_schedule) -> std::tuple<bool, std::string> {
    // Split on space to get each separate part, six parts expected
    const std::regex split{R"#(^\s*(.*?)\s+(.*?)\s+(.*?)\s+(.*?)\s+(.*?)\s+(.*?)\s*$)#",
                           std::regex_constants::ECMAScript};

    std::smatch all_sections;
    auto res = std::regex_match(cron_schedule.cbegin(), cron_schedule.cend(), all_sections, split);

    // Replace text with numbers
    std::string working_copy{};

    if (res) {
        // Merge seconds, minutes, hours and day of month back together
        working_copy += all_sections[1].str();
        working_copy += " ";
        working_copy += all_sections[2].str();
        working_copy += " ";
        working_copy += all_sections[3].str();
        working_copy += " ";
        working_copy += all_sections[4].str();
        working_copy += " ";

        // Replace month names
        auto month = all_sections[5].str();
        Data::ReplaceStringNameWithNumeric<Months>(month);

        working_copy += " ";
        working_copy += month;

        // Replace day names
        auto dow = all_sections[6].str();
        Data::ReplaceStringNameWithNumeric<DayOfWeek>(dow);

        working_copy += " ";
        working_copy += dow;
    }

    std::string final_cron_schedule{};

    // Split again on space
    res = res && std::regex_match(working_copy.cbegin(), working_copy.cend(), all_sections, split);

    if (res) {
        int selected_value = -1;
        auto second = GetRandomInRange<Seconds>(all_sections[1].str(), selected_value);
        res = second.first;
        final_cron_schedule = second.second;

        auto minute = GetRandomInRange<Minutes>(all_sections[2].str(), selected_value);
        res &= minute.first;
        final_cron_schedule += " " + minute.second;

        auto hour = GetRandomInRange<Hours>(all_sections[3].str(), selected_value);
        res &= hour.first;
        final_cron_schedule += " " + hour.second;

        // Do Month before DayOfMonth to allow capping the allowed range.
        auto month = GetRandomInRange<Months>(all_sections[5].str(), selected_value);
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

        auto day_of_month = GetRandomInRange<DayOfMonth>(all_sections[4].str(), selected_value, limits);

        res &= day_of_month.first;
        final_cron_schedule += " " + day_of_month.second + " " + month.second;

        auto day_of_week = GetRandomInRange<DayOfWeek>(all_sections[6].str(), selected_value);
        res &= day_of_week.first;
        final_cron_schedule += " " + day_of_week.second;
    }

    return {res, final_cron_schedule};
}

auto Randomization::DayLimiter(const std::set<Months>& months) -> std::pair<int, int> {
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

    auto res = std::pair<int, int>{Data::ValueOf(DayOfMonth::First), max};

    return res;
}

auto Randomization::Cap(int value, int lower, int upper) -> int { return std::max(std::min(value, upper), lower); }
}  // namespace oryx::chron
