#include <oryx/chron/parser.hpp>

#include <optional>

#include <oryx/chron/details/parser.hpp>
#include <oryx/chron/preprocessor.hpp>
#include <oryx/chron/common.hpp>

namespace oryx::chron {

auto ExpressionParser::operator()(std::string_view cron_expression) const -> std::optional<ChronData> {
    static constexpr auto matcher = ctre::match<R"#(^\s*(.*?)\s+(.*?)\s+(.*?)\s+(.*?)\s+(.*?)\s+(.*?)\s*$)#">;

    auto preprocessed =
        PreprocessExpression<DollarExpressionProcessor, WeekMonthDayLiteralProcessor>(std::string(cron_expression));
    auto match = matcher(std::move(preprocessed));
    if (!match) [[unlikely]] {
        return std::nullopt;
    }

    ChronData data{};
    bool valid = details::Parser::ValidateNumeric<Seconds>(match.get<1>().to_view(), data.seconds);
    valid &= details::Parser::ValidateNumeric<Minutes>(match.get<2>().to_view(), data.minutes);
    valid &= details::Parser::ValidateNumeric<Hours>(match.get<3>().to_view(), data.hours);
    valid &= details::Parser::ValidateNumeric<MonthDays>(match.get<4>().to_view(), data.days);
    valid &= details::Parser::ValidateNumeric<Months>(match.get<5>().to_view(), data.months);
    valid &= details::Parser::ValidateNumeric<Weekdays>(match.get<6>().to_view(), data.weeks);
    valid &= details::Parser::CheckDomVsDow(match.get<4>().to_view(), match.get<6>().to_view());
    valid &= details::Parser::ValidateDateVsMonths(data);

    if (!valid) [[unlikely]] {
        return std::nullopt;
    }
    return data;
}

template class ORYX_CHRON_API CachedExpressionParser<NullMutex>;
template class ORYX_CHRON_API CachedExpressionParser<std::mutex>;

static_assert(traits::Parser<ExpressionParser>);
static_assert(traits::Parser<CachedExpressionParser<NullMutex>>);

}  // namespace oryx::chron