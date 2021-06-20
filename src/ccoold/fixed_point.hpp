#pragma once

#include <cmath>
#include <cstdint>

namespace ccool {

template <std::size_t Width> struct FixedPointUnderlyingType {};

template <>
struct FixedPointUnderlyingType<16>
{
	using Type = std::uint16_t;

	constexpr static std::uint32_t IntegerBitShift = 8;
	constexpr static std::uint16_t IntegerMask = 0xFF00;
	constexpr static std::uint16_t FractionMask = 0x00FF;
};

template <std::size_t Width>
class FixedPoint
{
public:
	static constexpr auto width = Width;
	using UnderlyingType = typename FixedPointUnderlyingType<Width>::Type;

	constexpr static auto IntegerBitShift = FixedPointUnderlyingType<Width>::IntegerBitShift;
	constexpr static auto IntegerMask = FixedPointUnderlyingType<Width>::IntegerMask;
	constexpr static auto FractionMask = FixedPointUnderlyingType<Width>::FractionMask;

	FixedPoint() : _data(0) {}
	FixedPoint(UnderlyingType data) : _data(data) {}
	FixedPoint(double fp) : _data(0)
	{
		auto int_part = static_cast<UnderlyingType>(fp);
		auto frac_part = static_cast<UnderlyingType>(std::round((fp - static_cast<double>(int_part)) * 10.0));
		_data = ((int_part << IntegerBitShift) & IntegerMask) | (frac_part & FractionMask);
	}
	FixedPoint(const FixedPoint&) = default;
	FixedPoint(FixedPoint&&) noexcept = default;

	FixedPoint& operator=(const FixedPoint&) = default;
	FixedPoint& operator=(FixedPoint&&) noexcept = default;

	UnderlyingType data() const { return _data; }
	double floating() const { return static_cast<double>(integer()) + 0.1 * static_cast<double>(fraction()); }

	bool operator==(const FixedPoint<Width>& rhs) const { return _data == rhs._data; }
	bool operator!=(const FixedPoint<Width>& rhs) const { return !(*this == rhs); }

private:
	UnderlyingType integer() const { return (_data & IntegerMask) >> IntegerBitShift; }
	UnderlyingType fraction() const { return _data & FractionMask; }

	UnderlyingType _data;
};

} // namespace ccool
