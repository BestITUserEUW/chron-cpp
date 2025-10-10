#include <oryx/chron/parser.hpp>

#include <optional>

#include <oryx/chron/details/parser.hpp>

namespace oryx::chron {
namespace {

struct DollarExprPair {
    std::string_view expr;
    std::string_view cron;
};

}  // namespace

auto ExpressionParser::operator()(std::string_view cron_expression) const -> std::optional<ChronData> {
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

    bool valid{};
    ChronData data{};
    if (auto match = ctre::match<R"#(^\s*(.*?)\s+(.*?)\s+(.*?)\s+(.*?)\s+(.*?)\s+(.*?)\s*$)#">(cron_expression)) {
        valid = details::Parser::ValidateNumeric<Seconds>(match.get<1>().to_view(), data.seconds);
        valid &= details::Parser::ValidateNumeric<Minutes>(match.get<2>().to_view(), data.minutes);
        valid &= details::Parser::ValidateNumeric<Hours>(match.get<3>().to_view(), data.hours);
        valid &= details::Parser::ValidateNumeric<Days>(match.get<4>().to_view(), data.days);
        valid &=
            details::Parser::ValidateLiteral<Months>(match.get<5>().to_string(), data.months, details::kMonthNames);
        valid &= details::Parser::ValidateLiteral<Weeks>(match.get<6>().to_string(), data.weeks, details::kDayNames);
        valid &= details::Parser::CheckDomVsDow(match.get<4>().to_view(), match.get<6>().to_view());
        valid &= details::Parser::ValidateDateVsMonths(data);
    }

    if (!valid) {
        return std::nullopt;
    }
    return data;
}

}  // namespace oryx::chron