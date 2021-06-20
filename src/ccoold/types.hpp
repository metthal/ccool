#pragma once

#include <cstdint>

namespace ccool {

struct Color
{
	std::uint8_t red, green, blue;
};

struct PWM
{
	std::uint8_t value;
};

struct Temperature
{
	double temperature;
};

struct RPM
{
	std::uint16_t value;
};

struct Version
{
	std::uint8_t major, minor, patch;
};

} // namespace ccool
