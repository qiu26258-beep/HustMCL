#pragma once

#include <algorithm>
#include <string_view>

namespace hustmcl::core::detail {
inline bool validSha1(std::string_view value)
{
    return value.size() == 40 && std::all_of(value.begin(), value.end(), [](char ch) {
        return (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f');
    });
}
} // namespace hustmcl::core::detail
