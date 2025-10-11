#pragma once

#include <optional>
#include <mutex>
#include <algorithm>

#include <oryx/chron/details/null_mutex.hpp>

#include "chron_data.hpp"
#include "traits.hpp"

namespace oryx::chron {

struct ExpressionParser {
    auto operator()(std::string_view cron_expression) const -> std::optional<ChronData>;
};

template <traits::BasicLockable MutexType = details::NullMutex>
class CachedExpressionParser : ExpressionParser {
public:
    using Pair = std::pair<std::size_t, ChronData>;

    auto operator()(std::string_view cron_expression) const -> std::optional<ChronData> {
        std::lock_guard lock{mtx_};
        auto it = std::ranges::find(cache_, hash_(cron_expression), &Pair::first);
        if (it != cache_.end()) {
            return it->second;
        }

        auto data = ExpressionParser::operator()(cron_expression);
        if (data) cache_.emplace_back(hash_(cron_expression), data.value());
        return data;
    }

    void Clear() {
        std::lock_guard lock{mtx_};
        cache_.clear();
    }

    auto Contains(std::string_view cron_expression) const -> bool {
        std::lock_guard lock{mtx_};
        return std::ranges::find(cache_, hash_(cron_expression), &Pair::first) != cache_.end();
    }

    auto GetSize() const -> size_t {
        std::lock_guard lock{mtx_};
        return cache_.size();
    }

private:
    std::hash<std::string_view> hash_{};
    mutable MutexType mtx_{};
    mutable std::vector<Pair> cache_{};
};

static_assert(traits::Parser<ExpressionParser>);
static_assert(traits::Parser<CachedExpressionParser<>>);

inline constexpr ExpressionParser kParseExpression{};

}  // namespace oryx::chron