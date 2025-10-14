#include <oryx/chron/preprocessor.hpp>

#include <algorithm>
#include <string_view>
#include <span>

#include <oryx/chron/details/ctre.hpp>
#include <oryx/chron/details/to_underlying.hpp>

namespace oryx::chron {
namespace {

struct DollarExpressionPair {
    std::string_view expr;
    std::string_view cron;
};

template <typename Enum>
    requires std::is_enum_v<Enum>
auto ReplaceWithNumeric(std::string data, std::span<const std::string_view> names) -> std::string {
    static auto find_icase = [](std::string_view haystack, std::string_view needle) {
        return std::ranges::search(
            haystack, needle, [](const int lhs, const int rhs) { return lhs == rhs; },
            [](const int lhs) { return std::toupper(lhs); });
    };

    auto value = details::to_underlying(Enum::First);

    std::string cached_str_value;
    for (auto& name : names) {
        std::string_view view{data};
        size_t search_start = 0;

        while (search_start < view.size()) {
            auto subview = view.substr(search_start);
            auto found = find_icase(subview, name);

            if (found.empty()) {
                break;
            }

            if (cached_str_value.empty()) {
                cached_str_value = std::to_string(value);
            }

            auto found_pos = search_start + std::distance(subview.begin(), found.begin());
            data.replace(found_pos, name.size(), cached_str_value);

            // Update part_view to reflect the changes and move search position
            view = data;
            search_start = found_pos + cached_str_value.size();
        }
        cached_str_value.clear();
        value++;
    }
    return data;
}

}  // namespace

auto DollarExpressionProcessor::Process(std::string data) noexcept -> std::string {
    static constexpr std::array<DollarExpressionPair, 6> kExpressions{
        DollarExpressionPair("@yearly", "0 0 0 1 1 *"),  DollarExpressionPair("@annually", "0 0 0 1 1 *"),
        DollarExpressionPair("@monthly", "0 0 0 1 * *"), DollarExpressionPair("@weekly", "0 0 0 * * 0"),
        DollarExpressionPair("@daily", "0 0 0 * * ?"),   DollarExpressionPair("@hourly", "0 0 * * * ?")};

    if (!data.empty() && data[0] == '@') {
        auto it = std::ranges::find(kExpressions, data, &DollarExpressionPair::expr);
        if (it != kExpressions.end()) [[likely]] {
            return std::string(it->cron);
        }
    }
    return data;
}

auto WeekMonthDayLiteralProcessor::Process(std::string data) noexcept -> std::string {
    static constexpr std::array<std::string_view, 12> kMonthNames{"JAN", "FEB", "MAR", "APR", "MAY", "JUN",
                                                                  "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

    static constexpr std::array<std::string_view, 7> kDayNames{"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
    static constexpr auto matcher = ctre::match<R"#(^\s*(.*?)\s+(.*?)\s+(.*?)\s+(.*?)\s+(.*?)\s+(.*?)\s*$)#">;

    auto match = matcher(data);
    if (!match) [[unlikely]] {
        return data;
    }

    auto month = ReplaceWithNumeric<Months>(match.get<5>().to_string(), kMonthNames);
    auto dow = ReplaceWithNumeric<Weekdays>(match.get<6>().to_string(), kDayNames);
    return std::format("{} {} {} {} {} {}", match.get<1>().to_view(), match.get<2>().to_view(),
                       match.get<3>().to_view(), match.get<4>().to_view(), month, dow);
}

}  // namespace oryx::chron