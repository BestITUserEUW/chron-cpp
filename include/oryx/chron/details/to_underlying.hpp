#pragma once

#include <utility>
#include <type_traits>

namespace oryx::chron::details {

#ifndef __cpp_lib_to_underlying

template <typename Enum>
    requires std::is_enum_v<Enum>
constexpr auto to_underlying(Enum e) noexcept -> std::underlying_type_t<Enum> {
    return static_cast<std::underlying_type_t<Enum>>(e);
}

#else

using std::to_underlying;

#endif

}  // namespace oryx::chron::details