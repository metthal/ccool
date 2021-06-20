#include "string.hpp"

namespace ccool {

std::vector<std::string_view> split(std::string_view str, char delim)
{
	std::vector<std::string_view> result;

	std::string::size_type pos = 0, last_pos = 0;
	while ((pos = str.find(delim, last_pos)) != std::string::npos)
	{
		result.emplace_back(str.begin() + last_pos, str.begin() + pos);
		last_pos = pos + 1;
	}

	result.emplace_back(str.begin() + last_pos, str.end());
	return result;
}

} // namespace ccool
