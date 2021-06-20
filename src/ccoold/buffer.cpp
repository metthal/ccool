#include <cctype>
#include <stdexcept>

#include "buffer.hpp"

namespace ccool {

namespace {

inline char byte_to_hex_char(std::uint8_t byte)
{
	byte = byte & 0x0F;
	if (byte <= 9)
		return byte + '0';
	else
		return byte - 10 + 'a';
}

inline std::uint8_t hex_char_to_nibble(char hex_char)
{
	hex_char = std::tolower(hex_char);
	if ('0' <= hex_char && hex_char <= '9')
		return hex_char - '0';
	else if ('a' <= hex_char && hex_char <= 'f')
		return hex_char - 'a' + 10;
	else
		throw std::runtime_error("Invalid hex char when converting to bytes");
}

inline std::uint8_t hex_chars_to_byte(char high_char, char low_char)
{
	return (hex_char_to_nibble(high_char) << 4) | hex_char_to_nibble(low_char);
}

}

BytesView operator "" _bv(const char* bytes, std::size_t size)
{
	return {reinterpret_cast<const std::uint8_t*>(bytes), size};
}

Buffer::Buffer()  : _write_pos(0), _read_pos(0), _data()
{
}

Buffer::Buffer(BytesView data) : _write_pos(data.end() - data.begin()), _read_pos(0), _data(data.begin(), data.end())
{
}

Buffer::Buffer(std::size_t size) : _write_pos(size), _read_pos(0), _data(size, 0)
{
}

Buffer::Buffer(const std::uint8_t* data, std::size_t size) : _write_pos(size), _read_pos(0), _data(data, data + size)
{
}

Buffer::Buffer(std::vector<std::uint8_t>&& data) : _write_pos(data.size()), _read_pos(0), _data(std::move(data))
{
}

Buffer::Buffer(const std::string& hex_string) : _write_pos(0), _read_pos(0), _data()
{
	_data.reserve(hex_string.size() / 2);
	for (std::size_t i = 0; i < (hex_string.size() & ~1); i += 2)
		_data.push_back(hex_chars_to_byte(hex_string[i], hex_string[i + 1]));
	_write_pos = _data.size();
}

std::uint8_t* Buffer::get_raw_data()
{
	return _data.data();
}

const std::uint8_t* Buffer::get_raw_data() const
{
	return _data.data();
}

BytesView Buffer::get_data() const
{
	return {_data.data(), _data.size()};
}

std::size_t Buffer::get_size() const
{
	return _data.size();
}

void Buffer::resize(std::size_t new_size)
{
	_data.resize(new_size, 0);
}

std::string Buffer::get_hex_string() const
{
	std::string result(get_size() * 2, '\0');
	for (std::size_t i = 0; i < get_size(); ++i)
	{
		result[2*i] = byte_to_hex_char(_data[i] >> 4);
		result[2*i + 1] = byte_to_hex_char(_data[i]);
	}
	return result;
}

bool Buffer::operator==(const Buffer& rhs) const
{
	return _data == rhs._data;
}

bool Buffer::operator!=(const Buffer& rhs) const
{
	return !(*this == rhs);
}

} // namespace ccool
