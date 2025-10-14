#pragma once

#include <string_view>
#include <optional>
#include <random>

#include "common.hpp"

namespace oryx::chron {

class ORYX_CHRON_API Randomization {
public:
    Randomization();

    Randomization(const Randomization&) = delete;
    auto operator=(const Randomization&) -> Randomization& = delete;

    auto Parse(std::string_view cron_schedule) -> std::optional<std::string>;

private:
    std::random_device random_device_;
    std::mt19937 twister_;
};

}  // namespace oryx::chron
