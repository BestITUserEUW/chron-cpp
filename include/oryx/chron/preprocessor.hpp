#pragma once

#include "traits.hpp"

namespace oryx::chron {

template <traits::Processor... Ps>
struct Processors;

template <>
struct Processors<> {
    static void Process(std::string&) noexcept {}
};

template <traits::Processor Curr, traits::Processor... Rest>
struct Processors<Curr, Rest...> {
    static auto Process(std::string data) noexcept -> std::string {
        if constexpr (sizeof...(Rest) == 0)
            return Curr::Process(std::move(data));
        else
            return Processors<Rest...>::Process(Curr::Process(std::move(data)));
    }
};

struct ORYX_CHRON_API DollarExpressionProcessor {
    static auto Process(std::string data) noexcept -> std::string;
};

struct ORYX_CHRON_API WeekMonthDayLiteralProcessor {
    static auto Process(std::string data) noexcept -> std::string;
};

template <traits::Processor... Ps>
auto PreprocessExpression(std::string data) noexcept -> std::string {
    return Processors<Ps...>::Process(data);
}

static_assert(traits::Processor<DollarExpressionProcessor>);
static_assert(traits::Processor<WeekMonthDayLiteralProcessor>);

}  // namespace oryx::chron