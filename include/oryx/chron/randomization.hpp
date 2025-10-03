#pragma once

#include <string>
#include <set>
#include <tuple>
#include <regex>
#include <random>
#include <utility>

#include "data.hpp"

namespace oryx::chron {
class Randomization {
public:
    Randomization();

    // Delete copy operations
    Randomization(const Randomization&) = delete;
    auto operator=(const Randomization&) -> Randomization& = delete;

    auto Parse(const std::string& cron_schedule) -> std::tuple<bool, std::string>;

private:
    template <typename T>
    auto GetRandomInRange(const std::string& section,
                          int& selected_value,
                          std::pair<int, int> limit = std::make_pair(-1, -1)) -> std::pair<bool, std::string>;

    auto DayLimiter(const std::set<Months>& month) -> std::pair<int, int>;
    auto Cap(int value, int lower, int upper) -> int;

    // Members
    const std::regex rand_expression_{R"#([rR]\((\d+)\-(\d+)\))#", std::regex_constants::ECMAScript};
    std::random_device random_device_;
    std::mt19937 twister_;
};

template <typename T>
auto Randomization::GetRandomInRange(const std::string& section, int& selected_value, std::pair<int, int> limit)
    -> std::pair<bool, std::string> {
    auto result = std::make_pair(true, std::string{});
    selected_value = -1;

    std::smatch random_match;

    if (std::regex_match(section.cbegin(), section.cend(), random_match, rand_expression_)) {
        // Random range, parse left and right numbers
        auto left = std::stoi(random_match[1].str());
        auto right = std::stoi(random_match[2].str());

        // Apply limit if provided
        if (limit.first != -1 && limit.second != -1) {
            left = Cap(left, limit.first, limit.second);
            right = Cap(right, limit.first, limit.second);
        }

        Data cron_data;
        std::set<T> numbers;
        result.first = cron_data.ConvertFromStringRangeToNumberRange<T>(
            std::to_string(left) + "-" + std::to_string(right), numbers);

        // Remove items outside the limit
        if (limit.first != -1 && limit.second != -1) {
            for (auto it = numbers.begin(); it != numbers.end();) {
                if (Data::ValueOf(*it) < limit.first || Data::ValueOf(*it) > limit.second) {
                    it = numbers.erase(it);
                } else {
                    ++it;
                }
            }
        }

        if (result.first && !numbers.empty()) {
            // Select a random value from the valid numbers
            std::uniform_int_distribution<> distribution(0, static_cast<int>(numbers.size() - 1));
            auto it = numbers.begin();
            std::advance(it, distribution(twister_));
            selected_value = Data::ValueOf(*it);
            result.second = std::to_string(selected_value);
        }
    } else {
        // Not a random section, return as-is
        result.second = section;
    }

    return result;
}
}  // namespace oryx::chron
