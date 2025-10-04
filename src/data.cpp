#include <oryx/chron/data.hpp>

#include <algorithm>

namespace oryx::chron {

// FIX_ME: Not thread safe
std::unordered_map<std::string, Data> Data::cache_{};

auto Data::Create(const std::string& cron_expression) -> Data {
    Data c;
    auto found = cache_.find(cron_expression);

    if (found == cache_.end()) {
        c.Parse(cron_expression);
        cache_[cron_expression] = c;
    } else {
        c = found->second;
    }

    return c;
}

void Data::Parse(const std::string& cron_expression) {
    static const std::regex kSplit{R"#(^\s*(.*?)\s+(.*?)\s+(.*?)\s+(.*?)\s+(.*?)\s+(.*?)\s*$)#",
                                   std::regex_constants::ECMAScript};
    static const std::unordered_map<std::string, std::string> shortcuts{
        {"@yearly", "0 0 0 1 1 *"}, {"@annually", "0 0 0 1 1 *"}, {"@monthly", "0 0 0 1 * *"},
        {"@weekly", "0 0 0 * * 0"}, {"@daily", "0 0 0 * * ?"},    {"@hourly", "0 0 * * * ?"}};

    // First, check for "convenience scheduling" using @yearly, @annually,
    // @monthly, @weekly, @daily or @hourly.
    std::string expression = cron_expression;
    if (!expression.empty() && expression[0] == '@') {
        auto it = shortcuts.find(expression);
        if (it != shortcuts.end()) expression = it->second;
    }

    // Second, split on white-space. We expect six parts.
    std::smatch match;
    if (std::regex_match(expression.cbegin(), expression.cend(), match, kSplit)) {
        valid_ = ValidateNumeric<Seconds>(match[1], seconds_);
        valid_ &= ValidateNumeric<Minutes>(match[2], minutes_);
        valid_ &= ValidateNumeric<Hours>(match[3], hours_);
        valid_ &= ValidateNumeric<DayOfMonth>(match[4], day_of_month_);
        valid_ &= ValidateLiteral<Months>(match[5], months_, kMonthNames);
        valid_ &= ValidateLiteral<DayOfWeek>(match[6], day_of_week_, kDayNames);
        valid_ &= CheckDomVsDow(match[4], match[6]);
        valid_ &= ValidateDateVsMonths();
    }
}

auto Data::Split(const std::string& input, char delim) -> std::vector<std::string> {
    std::vector<std::string> result;
    size_t delim_pos;
    size_t pos{};

    while ((delim_pos = input.find(delim, pos)) != std::string_view::npos) {
        result.emplace_back(input.substr(pos, delim_pos - pos));
        pos = delim_pos + 1;
    }
    result.emplace_back(input.substr(pos));  // Add the last part
    return result;
}

auto Data::IsNumber(const std::string& s) -> bool { return !s.empty() && std::ranges::all_of(s, ::isdigit); }

auto Data::IsBetween(int32_t value, int32_t low_limit, int32_t high_limt) -> bool {
    return value >= low_limit && value <= high_limt;
}

auto Data::ValidateDateVsMonths() const -> bool {
    // Only February allowed? Ensure day_of_month includes only 1..29
    if (months_.size() == 1 && months_.count(static_cast<Months>(2))) {
        if (!HasAnyInRange(day_of_month_, 1, 29)) return false;
    }

    // If only day 31 is selected, ensure at least one month allows it
    if (day_of_month_.size() == 1 && day_of_month_.count(DayOfMonth::Last)) {
        if (!std::ranges::any_of(kMonthsWith31, [this](Months m) { return months_.count(m) > 0; })) {
            return false;
        }
    }

    return true;
}

auto Data::CheckDomVsDow(const std::string& dom, const std::string& dow) const -> bool {
    // Day of month and day of week are mutually exclusive so one of them must at always be ignored using
    // the '?'-character unless one field already is something other than '*'.
    //
    // Since we treat an ignored field as allowing the full range, we're OK with both being flagged
    // as ignored. To make it explicit to the user of the library, we do however require the use of
    // '?' as the ignore flag, although it is functionally equivalent to '*'.

    auto check = [](const std::string& l, const std::string& r) { return l == "*" && (r != "*" || r == "?"); };

    return (dom == "?" || dow == "?") || check(dom, dow) || check(dow, dom);
}
}  // namespace oryx::chron
