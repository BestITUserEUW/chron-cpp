#pragma once

#include <chrono>

namespace oryx::chron {

using Clock = std::chrono::system_clock;
using TimePoint = std::chrono::time_point<Clock>;
using Duration = Clock::duration;

}  // namespace oryx::chron