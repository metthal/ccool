#pragma once

#include <iostream>

#include <charconv>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>

#include <fmt/format.h>

namespace ccool {

namespace detail {

template <typename T, bool v = std::is_integral_v<T>>
struct is_int_excluding_bool
{
	static constexpr bool value = v;
};

template <>
struct is_int_excluding_bool<bool>
{
	static constexpr bool value = false;
};

template <typename T>
static constexpr bool is_int_excluding_bool_v = is_int_excluding_bool<T>::value;

template <typename T>
std::optional<T> sequence_to(const char* data, std::size_t length)
{
	T result = {};
	auto [ptr, error_code] = std::from_chars(data, data + length, result);
	if (error_code == std::errc() && (!ptr || ptr != data + length))
		return std::nullopt;
	return result;
}

template <typename From, typename To, typename = void>
struct Converter {};

template <typename From, typename To>
struct Converter<From, To, std::enable_if_t<
		(std::is_integral_v<To> && std::is_integral_v<From> && std::is_convertible_v<To, From>)
		|| (!std::is_integral_v<To> && !std::is_integral_v<From> && std::is_constructible_v<To, From>)
	, void>>
{
	static std::optional<To> convert(const From& from) { return To{from}; }
};

template <typename T>
struct Converter<T, std::string_view, std::enable_if_t<is_int_excluding_bool_v<T>, void>>
{
	static std::optional<std::string> convert(const T& from) { return fmt::format("{}", from); }
};

template <typename T>
struct Converter<T, std::string, std::enable_if_t<is_int_excluding_bool_v<T>, void>>
{
	static std::optional<std::string> convert(const T& from) { return fmt::format("{}", from); }
};

template <typename T>
struct Converter<std::string_view, T, std::enable_if_t<is_int_excluding_bool_v<T>, void>>
{
	static std::optional<T> convert(const std::string_view& from) { return sequence_to<T>(from.data(), from.length()); }
};

template <typename T>
struct Converter<std::string, T, std::enable_if_t<is_int_excluding_bool_v<T>, void>>
{
	static std::optional<T> convert(const std::string& from) { return sequence_to<T>(from.data(), from.length()); }
};

template <typename T>
struct Converter<const char*, T, std::enable_if_t<is_int_excluding_bool_v<T>, void>>
{
	static std::optional<T> convert(const char* from) { return sequence_to<T>(from, std::strlen(from)); }
};

template <typename T, std::size_t N>
struct Converter<char[N], T, std::enable_if_t<is_int_excluding_bool_v<T>, void>>
{
	static std::optional<T> convert(const char (&from)[N]) { return sequence_to<T>(from, N); }
};

template <>
struct Converter<bool, std::string, void>
{
	static std::optional<std::string> convert(bool from) { return from ? "true" : "false"; }
};

template <std::size_t N>
struct Converter<char[N], bool, void>
{
	static std::optional<bool> convert(const char (&from)[N]) { return from == std::string_view{"true"} || from == std::string_view{"1"}; }
};

template <>
struct Converter<std::string_view, bool, void>
{
	static std::optional<bool> convert(const std::string_view& from) { return from == "true" || from == "1"; }
};

template <>
struct Converter<std::string, bool, void>
{
	static std::optional<bool> convert(const std::string& from) { return from == "true" || from == "1"; }
};

} // namespace detail

template <typename To, typename From>
std::optional<To> convert(const From& from)
{
	return detail::Converter<std::remove_cv_t<std::remove_reference_t<From>>, To>::convert(from);
}

} // namespace ccool
