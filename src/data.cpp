
#include <oryx/chron/data.hpp>

namespace oryx::chron {

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
    // First, check for "convenience scheduling" using @yearly, @annually,
    // @monthly, @weekly, @daily or @hourly.
    std::string tmp = std::regex_replace(cron_expression, std::regex("@yearly"), "0 0 1 1 *");
    tmp = std::regex_replace(tmp, std::regex("@annually"), "0 0 1 1 *");
    tmp = std::regex_replace(tmp, std::regex("@monthly"), "0 0 1 * *");
    tmp = std::regex_replace(tmp, std::regex("@weekly"), "0 0 * * 0");
    tmp = std::regex_replace(tmp, std::regex("@daily"), "0 0 * * *");
    const std::string expression = std::regex_replace(tmp, std::regex("@hourly"), "0 * * * *");

    // Second, split on white-space. We expect six parts.
    std::regex split{R"#(^\s*(.*?)\s+(.*?)\s+(.*?)\s+(.*?)\s+(.*?)\s+(.*?)\s*$)#", std::regex_constants::ECMAScript};

    std::smatch match;

    if (std::regex_match(expression.begin(), expression.end(), match, split)) {
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

auto Data::IsNumber(const std::string& s) -> bool {
    // Find any character that isn't a number.
    return !s.empty() && std::find_if(s.begin(), s.end(), [](char c) { return !std::isdigit(c); }) == s.end();
}

auto Data::IsBetween(int32_t value, int32_t low_limit, int32_t high_limt) -> bool {
    return value >= low_limit && value <= high_limt;
}

auto Data::ValidateDateVsMonths() const -> bool {
    bool res = true;

    // Verify that the available dates are possible based on the given months
    if (months_.size() == 1 && months_.find(static_cast<Months>(2)) != months_.end()) {
        // Only february allowed, make sure that the allowed date(s) includes 29 and below.
        res = HasAnyInRange(day_of_month_, 1, 29);
    }

    if (res) {
        // Make sure that if the days contains only 31, at least one month allows that date.
        if (day_of_month_.size() == 1 && day_of_month_.find(DayOfMonth::Last) != day_of_month_.end()) {
            res = false;

            for (size_t i = 0; !res && i < kNumberOfLongMonths; ++i) {
                res = months_.find(kMonthsWith31[i]) != months_.end();
            }
        }
    }

    return res;
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
