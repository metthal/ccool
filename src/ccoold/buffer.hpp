#pragma once

#include <cstdint>
#include <cstring>
#include <optional>
#include <string_view>
#include <vector>

#include "endian.hpp"
#include "fixed_point.hpp"

namespace ccool {

using BytesView = std::basic_string_view<std::uint8_t>;

BytesView operator "" _bv(const char* bytes, std::size_t size);

class Buffer
{
public:
	Buffer();
	Buffer(BytesView data);
	Buffer(std::size_t size);
	Buffer(const std::uint8_t* data, std::size_t size);
	Buffer(std::vector<std::uint8_t>&& data);
	Buffer(const std::string& hex_string);
	Buffer(const Buffer&) = default;
	Buffer(Buffer&&) noexcept = default;

	Buffer& operator=(const Buffer&) = default;
	Buffer& operator=(Buffer&&) noexcept = default;

	std::uint8_t* get_raw_data();
	const std::uint8_t* get_raw_data() const;
	BytesView get_data() const;
	std::size_t get_size() const;

	void resize(std::size_t new_size);

	template <Endian E, typename T>
	std::enable_if_t<std::is_integral_v<T>, void> write(T value)
	{
		if (_write_pos + sizeof(T) >= _data.size())
			_data.resize(_data.size() + sizeof(T));

		auto value_ce = endian_convert<Endian::Native, E>(value);
		std::memcpy(_data.data() + _write_pos, reinterpret_cast<std::uint8_t*>(&value_ce), sizeof(T));
		_write_pos += sizeof(T);
	}

	template <Endian E, typename T, std::size_t Width = T::width>
	void write(const T& value)
	{
		write<E, typename T::UnderlyingType>(value.data());
	}

	template <Endian E, typename T, typename InnerT = typename T::value_type>
	void write(const T& values)
	{
		for (auto&& value : values)
			write<E, typename T::value_type>(value);
	}

	void write_nt_string(std::string_view str)
	{
		if (_write_pos >= _data.size())
			_data.resize(_data.size() + str.size() + 1);

		std::strcpy(reinterpret_cast<char*>(_data.data()) + _write_pos, str.data());
		_write_pos += str.size() + 1;
	}

	template <Endian E, typename T>
	std::enable_if_t<std::is_integral_v<T>, std::optional<T>> read() const
	{
		if (_read_pos + sizeof(T) > _data.size())
			return std::nullopt;

		T result;
		std::memcpy(reinterpret_cast<std::uint8_t*>(&result), _data.data() + _read_pos, sizeof(T));
		_read_pos += sizeof(T);
		return endian_convert<E, Endian::Native>(result);
	}

	template <Endian E, typename T, std::size_t Width = T::width>
	std::enable_if_t<std::is_same_v<T, FixedPoint<Width>>, std::optional<T>> read() const
	{
		auto result_fp = read<E, typename T::UnderlyingType>();
		if (!result_fp)
			return std::nullopt;

		return FixedPoint<Width>(result_fp.value());
	}

	template <Endian E, typename T, typename InnerT = typename T::value_type>
	std::enable_if_t<std::is_same_v<T, std::vector<InnerT>>, std::optional<T>> read(std::size_t count) const
	{
		T result;
		result.reserve(count);

		for (std::size_t i = 0; i < count; ++i)
		{
			if (auto value = read<E, InnerT>(); value.has_value())
				result.push_back(std::move(value.value()));
		}

		return result;
	}

	std::optional<std::string_view> read_nt_string() const
	{
		auto pos = _read_pos;
		while (pos < _data.size())
		{
			if (_data[pos++] == '\0')
				return std::string_view{reinterpret_cast<const char*>(_data.data()) + _read_pos};
		}

		_read_pos = pos;
		return std::nullopt;
	}

	std::string get_hex_string() const;

	bool operator==(const Buffer& rhs) const;
	bool operator!=(const Buffer& rhs) const;

private:
	mutable std::size_t _write_pos, _read_pos;
	std::vector<std::uint8_t> _data;
};

} // namespace ccool
