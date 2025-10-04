#include <oryx/chron/version.hpp>

#include <format>

namespace oryx::chron {

auto MakeStringVersion() -> std::string { return std::format("{}.{}.{}", kVersionMajor, KVersionMinor, kVersionPatch); }

}  // namespace oryx::chron