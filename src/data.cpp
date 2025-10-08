#include <oryx/chron/data.hpp>

#include <algorithm>
#include <unordered_map>
#include <mutex>

#include <oryx/chron/details/any_of.hpp>
#include <oryx/chron/details/ctre.hpp>

namespace oryx::chron {
namespace {

struct DollarExprPair {
    std::string_view expr;
    std::string_view cron;
};

auto CheckDomVsDow(std::string_view dom, std::string_view dow) -> bool {
    // Day of month and day of week are mutually exclusive so one of them must at always be ignored using
    // the '?'-character unless one field already is something other than '*'.
    //
    // Since we treat an ignored field as allowing the full range, we're OK with both being flagged
    // as ignored. To make it explicit to the user of the library, we do however require the use of
    // '?' as the ignore flag, although it is functionally equivalent to '*'.

    auto check = [](std::string_view l, std::string_view r) { return l == "*" && (r != "*" || r == "?"); };
    return (dom == "?" || dow == "?") || check(dom, dow) || check(dow, dom);
}

std::mutex data_cache_mtx;
std::unordered_map<std::string, Data> data_cache;

}  // namespace

template <>
auto Data::Create<DataCachePolicy::kUseCache>(const std::string& cron_expression) -> Data {
    Data data;
    auto found = data_cache.find(cron_expression);
    if (found == data_cache.end()) {
        data.Parse(cron_expression);
        data_cache[cron_expression] = data;
    } else {
        data = found->second;
    }

    return data;
}

template <>
auto Data::Create<DataCachePolicy::kUseCacheThreadSafe>(const std::string& cron_expression) -> Data {
    std::lock_guard lock{data_cache_mtx};
    return Create<DataCachePolicy::kUseCache>(cron_expression);
}

template <>
auto Data::Create<DataCachePolicy::kBypassCache>(const std::string& cron_expression) -> Data {
    Data data;
    data.Parse(cron_expression);
    return data;
}

void Data::Parse(std::string_view cron_expression) {
    static constexpr std::array<DollarExprPair, 6> kShortcuts{
        DollarExprPair("@yearly", "0 0 0 1 1 *"),  DollarExprPair("@annually", "0 0 0 1 1 *"),
        DollarExprPair("@monthly", "0 0 0 1 * *"), DollarExprPair("@weekly", "0 0 0 * * 0"),
        DollarExprPair("@daily", "0 0 0 * * ?"),   DollarExprPair("@hourly", "0 0 * * * ?")};

    if (!cron_expression.empty() && cron_expression[0] == '@') {
        auto it = std::ranges::find(kShortcuts, cron_expression, &DollarExprPair::expr);
        if (it != kShortcuts.end()) {
            cron_expression = it->cron;
        }
    }

    if (auto match = ctre::match<R"#(^\s*(.*?)\s+(.*?)\s+(.*?)\s+(.*?)\s+(.*?)\s+(.*?)\s*$)#">(cron_expression)) {
        valid_ = ValidateNumeric<Seconds>(match.get<1>().to_view(), seconds_);
        valid_ &= ValidateNumeric<Minutes>(match.get<2>().to_view(), minutes_);
        valid_ &= ValidateNumeric<Hours>(match.get<3>().to_view(), hours_);
        valid_ &= ValidateNumeric<DayOfMonth>(match.get<4>().to_view(), day_of_month_);
        valid_ &= ValidateLiteral<Months>(match.get<5>().to_string(), months_, details::kMonthNames);
        valid_ &= ValidateLiteral<DayOfWeek>(match.get<6>().to_string(), day_of_week_, details::kDayNames);
        valid_ &= CheckDomVsDow(match.get<4>().to_view(), match.get<6>().to_view());
        valid_ &= ValidateDateVsMonths();
    }
}

auto Data::IsNumber(std::string_view s) -> bool { return !s.empty() && std::ranges::all_of(s, ::isdigit); }

auto Data::IsBetween(int32_t value, int32_t low_limit, int32_t high_limt) -> bool {
    return value >= low_limit && value <= high_limt;
}

auto Data::ValidateDateVsMonths() const -> bool {
    // Only February allowed? Ensure day_of_month includes only 1..29
    if (months_.size() == 1 && months_.count(static_cast<Months>(2))) {
        if (!details::AnyOf(day_of_month_, 1, 29)) return false;
    }

    // If only day 31 is selected, ensure at least one month allows it
    if (day_of_month_.size() == 1 && day_of_month_.count(DayOfMonth::Last)) {
        if (!std::ranges::any_of(kMonthsWith31, [this](Months m) { return months_.count(m) > 0; })) {
            return false;
        }
    }

    return true;
}

}  // namespace oryx::chron
