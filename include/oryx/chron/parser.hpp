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
class CachedExpressionParser {
public:
    using Pair = std::pair<std::string, ChronData>;

    auto operator()(std::string_view cron_expression) const -> std::optional<ChronData> {
        std::lock_guard lock{mtx_};
        auto it = std::ranges::find(cache_, cron_expression, &Pair::first);
        if (it != cache_.end()) {
            return it->second;
        }

        auto data = parser_(cron_expression);
        if (data) cache_.emplace_back(std::string(cron_expression), data.value());
        return data;
    }

    void ClearCache() {
        std::lock_guard lock{mtx_};
        cache_.clear();
    }

    auto GetCacheSize() const -> size_t {
        std::lock_guard lock{mtx_};
        return cache_.size();
    }

private:
    ExpressionParser parser_{};
    mutable MutexType mtx_{};
    mutable std::vector<Pair> cache_{};
};

static_assert(traits::Parser<ExpressionParser>);
static_assert(traits::Parser<CachedExpressionParser<>>);

inline constexpr ExpressionParser kParseExpression{};

}  // namespace oryx::chron