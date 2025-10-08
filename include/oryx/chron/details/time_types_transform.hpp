#pragma once

#include <string>
#include <string_view>
#include <array>
#include <span>
#include <algorithm>

#include "oryx/chron/time_types.hpp"
#include "to_underlying.hpp"

namespace oryx::chron::details {

inline constexpr std::array<std::string_view, 12> kMonthNames{"JAN", "FEB", "MAR", "APR", "MAY", "JUN",
                                                              "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

inline constexpr std::array<std::string_view, 7> kDayNames{"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};

using NamesView = std::span<const std::string_view>;

template <typename T>
auto ReplaceWithNumeric(std::string& data, NamesView names) -> std::string& {
    static auto find_icase = [](std::string_view haystack, std::string_view needle) {
        return std::ranges::search(
            haystack, needle, [](const int lhs, const int rhs) { return lhs == rhs; },
            [](const int lhs) { return std::toupper(lhs); });
    };

    auto value = to_underlying(T::First);

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

inline auto ReplaceDayNameWithNumeric(std::string& data) -> std::string& {
    return ReplaceWithNumeric<DayOfWeek>(data, kDayNames);
}

inline auto ReplaceMonthNameWithNumeric(std::string& data) -> std::string& {
    return ReplaceWithNumeric<Months>(data, kMonthNames);
}

}  // namespace oryx::chron::details