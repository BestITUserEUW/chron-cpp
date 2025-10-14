#pragma once

#include <chrono>
#include <concepts>
#include <optional>
#include <string_view>
#include <type_traits>

#include "chrono_types.hpp"
#include "chron_data.hpp"

namespace oryx::chron::traits {

template <typename T>
concept Clock = requires(const T& t, TimePoint tp) {
    { t.Now() } -> std::same_as<TimePoint>;
    { t.UtcOffset(tp) } -> std::same_as<std::chrono::seconds>;
};

template <typename L>
concept BasicLockable = requires(L m) {
    m.lock();
    m.unlock();
};

template <typename T>
concept TimeType = std::is_enum_v<T> && requires {
    typename std::decay_t<decltype(T::First)>;
    typename std::decay_t<decltype(T::Last)>;
};

template <typename T>
concept Parser = requires(T t, std::string_view sv) {
    { t(sv) } -> std::same_as<std::optional<ChronData>>;
};

template <typename T>
concept Processor = requires(T t, std::string s) {
    { T::Process(s) };
};

}  // namespace oryx::chron::traits