#pragma once

#include <string>

#include "common.hpp"

namespace oryx::chron {

inline constexpr int kVersionMajor = 0;
inline constexpr int KVersionMinor = 3;
inline constexpr int kVersionPatch = 0;

ORYX_CHRON_API auto MakeStringVersion() -> std::string;

}  // namespace oryx::chron