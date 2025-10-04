#pragma once

#include <string>
#include <tuple>
#include <random>

namespace oryx::chron {
class Randomization {
public:
    Randomization();

    // Delete copy operations
    Randomization(const Randomization&) = delete;
    auto operator=(const Randomization&) -> Randomization& = delete;

    auto Parse(const std::string& cron_schedule) -> std::tuple<bool, std::string>;

private:
    std::random_device random_device_;
    std::mt19937 twister_;
};

}  // namespace oryx::chron
