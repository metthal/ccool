#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace ccool {

std::vector<std::string_view> split(std::string_view str, char delim);

} // namespace ccool
