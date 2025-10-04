#pragma once

namespace oryx::chron {

inline constexpr int kVersionMajor = 0;
inline constexpr int KVersionMinor = 2;
inline constexpr int kVersionPatch = 0;

auto MakeStringVersion() -> std::string;

}  // namespace oryx::chron