#pragma once

#include <vector>
#include <string>

namespace oryx::chron::details {

inline auto StringSplit(std::string_view input, char delim) -> std::vector<std::string> {
    std::vector<std::string> result;
    size_t delim_pos;
    size_t pos{};

    while ((delim_pos = input.find(delim, pos)) != std::string_view::npos) {
        result.emplace_back(input.substr(pos, delim_pos - pos));
        pos = delim_pos + 1;
    }
    result.emplace_back(input.substr(pos));  // Add the last part
    return result;
}

}  // namespace oryx::chron::details