#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <poll.h>
#include <cstring>
#include <optional>
#include <string_view>
#include <memory>
#include <stdexcept>
#include <unistd.h>
#include <sstream>
#include <algorithm>
#include <unordered_set>
#include <functional>
#include <thread>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <nlohmann/json.hpp>
#include <cstdint>

namespace ulocal {




inline char hex_to_nibble(char c)
{
	auto lc = std::tolower(c);
	if ('0' <= lc && lc <= '9')
		return (c - '0') & 0x0F;
	else if ('a' <= lc && lc <= 'f')
		return (c - 'a' + 10) & 0x0F;
	else
		throw std::runtime_error("Invalid nibble value");
}

inline char nibbles_to_char(char high, char low)
{
	return (hex_to_nibble(high) << 4) | hex_to_nibble(low);
}

inline char char_to_hex(char c, bool high)
{
	c = high ? ((c >> 4) & 0x0F) : (c & 0x0F);
	if (0 <= c && c <= 9)
		return c + '0';
	else
		return c - 10 + 'a';
}

inline char char_to_hex_high(char c) { return char_to_hex(c, true); }
inline char char_to_hex_low(char c) { return char_to_hex(c, false); }

inline std::string char_to_hex(char c)
{
	std::string result(2, 0);
	result[0] = char_to_hex_high(c);
	result[1] = char_to_hex_low(c);
	return result;
}

template <typename StrT>
std::string url_decode(const StrT& str)
{
	std::string result;
	result.reserve(str.length());

	auto pos = str.find('%');
	decltype(pos) old_pos = 0;
	while (pos != std::string::npos)
	{
		if (pos > old_pos)
			result.append(str, old_pos, pos - old_pos);

		if (pos + 2 >= str.length())
		{
			break;
		}
		else
		{
			result += nibbles_to_char(str[pos + 1], str[pos + 2]);
			old_pos = std::min(str.length(), pos + 3);
			pos = str.find('%', old_pos);
		}
	}

	if (old_pos != str.length())
		result.append(str, old_pos);

	return result;
}

template <typename StrT>
std::string url_encode(const StrT& str)
{
	std::string result;
	result.reserve(2 * str.length());

	for (char c : str)
	{
		if (('a' <= c && c <= 'z') ||
			('A' <= c && c <= 'Z') ||
			('0' <= c && c <= '9') ||
			c == '-' ||
			c == '_' ||
			c == '~' ||
			c == '.')
		{
			result += c;
		}
		else
		{
			result += '%' + char_to_hex(c);
		}
	}

	return result;
}

template <class T>
inline void hash_combine(std::size_t& seed, const T& v)
{
	std::hash<T> hasher;
	seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template <typename StrT>
std::string lowercase(const StrT& str)
{
	std::string result;
	result.reserve(size(str));
	std::transform(begin(str), end(str), begin(result), [](char c) {
		return std::tolower(c);
	});
	return result;
}

template <typename StrT1, typename StrT2>
bool icase_compare(const StrT1& str1, const StrT2& str2)
{
	return std::equal(
		std::begin(str1), std::end(str1),
		std::begin(str2), std::end(str2),
		[](auto c1, auto c2) { return std::tolower(c1) == std::tolower(c2); }
	);
}

template <typename StrT>
std::string lstrip(const StrT& str)
{
	auto pos = str.find_first_not_of(" \r\n\t\v");
	if (pos == std::string::npos)
		return str;
	return std::string{str.data() + pos, str.length() - pos};
}

struct CaseInsensitiveHash
{
	std::size_t operator()(const std::string& str) const
	{
		std::size_t seed = 0;
		for (auto c : str)
			hash_combine(seed, std::tolower(c));
		return seed;
	}
};

struct CaseInsensitiveCompare
{
	bool operator()(const std::string& str1, const std::string& str2) const
	{
		return icase_compare(str1, str2);
	}
};



namespace detail {

template <typename... Args>
struct ValueGetter {};

template <typename T>
struct ValueGetter<T>
{
	using allow = std::enable_if_t<std::is_integral_v<T>, void>;

	static T convert(const std::string& str) { return std::stol(str); }
};

template <>
struct ValueGetter<bool>
{
	static bool convert(const std::string& str) { return icase_compare(str, "true"); }
};

template <typename... Args>
struct ValueSetter {};

template <>
struct ValueSetter<char*>
{
	static std::string convert(char* value) { return value; }
};

template <>
struct ValueSetter<const char*>
{
	static std::string convert(const char* value) { return value; }
};

template <>
struct ValueSetter<std::string>
{
	static std::string convert(const std::string& value) { return value; }
};

template <>
struct ValueSetter<bool>
{
	static std::string convert(bool value) { return value ? "true" : "false"; }
};

template <typename T>
struct ValueSetter<T>
{
	using allow = std::enable_if_t<std::is_integral_v<T>, bool>;

	static std::string convert(T value) { return std::to_string(value); }
};

}

class KeyValue
{
public:
	template <typename Name, typename Value>
	KeyValue(Name&& name, Value&& value) : _name(std::forward<Name>(name)), _value(detail::ValueSetter<std::decay_t<Value>>::convert(value)) {}

	const std::string& get_name() const { return _name; }
	const std::string& get_value() const { return _value; }

	template <typename T>
	T get_value_as() const
	{
		return detail::ValueGetter<T>::convert(_value);
	}

	template <typename Value>
	void set_value(const Value& value)
	{
		_value = detail::ValueSetter<std::decay_t<Value>>::convert(value);
	}

private:
	std::string _name, _value;
};


using HttpHeader = KeyValue;




class HttpHeaderTable
{
public:
	HttpHeaderTable() : _headers(), _table() {}

	auto begin() const { return _headers.begin(); }
	auto end() const { return _headers.end(); }
	std::size_t size() const { return _headers.size(); }

	void clear()
	{
		_headers.clear();
		_table.clear();
	}

	bool has_header(const std::string& name) const
	{
		return _table.find(name) != _table.end();
	}

	HttpHeader* get_header(const std::string& name)
	{
		auto itr = _table.find(name);
		if (itr == _table.end())
			return nullptr;

		return &itr->second;
	}

	const HttpHeader* get_header(const std::string& name) const
	{
		auto itr = _table.find(name);
		if (itr == _table.end())
			return nullptr;

		return &itr->second;
	}

	template <typename T1, typename T2>
	void add_header(T1&& name, T2&& value)
	{
		auto itr = _table.find(name);
		if (itr == _table.end())
		{
			auto header = HttpHeader{std::forward<T1>(name), std::forward<T2>(value)};
			std::tie(itr, std::ignore) = _table.emplace(header.get_name(), std::move(header));
			_headers.push_back(&itr->second);
		}
	}

private:
	std::vector<const HttpHeader*> _headers;
	std::unordered_map<std::string, HttpHeader, CaseInsensitiveHash, CaseInsensitiveCompare> _table;
};



class HttpMessage
{
public:
	HttpMessage() : HttpMessage(std::string{}, std::string{}) {}
	HttpMessage(const std::string&& content) : HttpMessage(content, "text/plain") {}
	HttpMessage(std::string&& content) : HttpMessage(std::move(content), "text/plain") {}
	HttpMessage(const nlohmann::json& json) : HttpMessage(json.dump(), "application/json") {}

	template <typename Content, typename ContentType>
	HttpMessage(Content&& content, ContentType&& content_type)
		: HttpMessage(std::move(content), std::forward<ContentType>(content_type), HttpHeaderTable{}) {}

	template <typename Content, typename ContentType, typename Headers>
	HttpMessage(Content&& content, ContentType&& content_type, Headers&& headers)
		: _content(std::forward<Content>(content)), _content_type(std::forward<ContentType>(content_type)), _headers(std::forward<Headers>(headers))
	{
		if (!_content_type.empty() && !has_header("Content-Type"))
			add_header("Content-Type", std::forward<ContentType>(content_type));
	}

	HttpMessage(const HttpMessage&) = default;
	HttpMessage(HttpMessage&&) noexcept = default;
	virtual ~HttpMessage() = default;

	HttpMessage& operator=(const HttpMessage&) = default;
	HttpMessage& operator=(HttpMessage&&) noexcept = default;

	const std::string& get_content() const { return _content; }
	nlohmann::json get_json() const { return nlohmann::json::parse(_content); }
	const HttpHeaderTable& get_headers() const { return _headers; }
	const HttpHeader* get_header(const std::string& name) const { return _headers.get_header(name); }

	bool has_header(const std::string& name) const { return _headers.has_header(name); }

	template <typename Name, typename Value>
	void add_header(Name&& name, Value&& value)
	{
		_headers.add_header(std::forward<Name>(name), std::forward<Value>(value));
	}

	void calculate_content_length()
	{
		if (auto content_length = _headers.get_header("Content-Length"); !content_length && !_content.empty())
			_headers.add_header("Content-Length", _content.length());
	}

	virtual std::string dump() const = 0;

protected:
	std::string _content, _content_type;
	HttpHeaderTable _headers;
};


using UrlArg = KeyValue;





class UrlArgs
{
public:
	UrlArgs() : _args(), _table() {}

	auto begin() const { return _args.begin(); }
	auto end() const { return _args.end(); }
	std::size_t size() const { return _args.size(); }

	void clear()
	{
		_args.clear();
		_table.clear();
	}

	bool has_arg(const std::string& name) const
	{
		return _table.find(name) != _table.end();
	}

	const UrlArg* get_arg(const std::string& name) const
	{
		auto itr = _table.find(name);
		if (itr == _table.end())
			return nullptr;

		return &itr->second;
	}

	template <typename T1, typename T2>
	void add_arg(T1&& name, T2&& value)
	{
		auto itr = _table.find(name);
		if (itr == _table.end())
		{
			auto header = UrlArg{std::forward<T1>(name), std::forward<T2>(value)};
			std::tie(itr, std::ignore) = _table.emplace(header.get_name(), std::move(header));
			_args.push_back(&itr->second);
		}
	}

	static std::pair<std::string, UrlArgs> parse_from_resource(std::string_view resource)
	{
		UrlArgs result;

		auto url_params_start = resource.find('?');
		if (url_params_start != std::string::npos)
		{
			auto url_args = std::string_view{resource.data() + url_params_start + 1, resource.length() - url_params_start - 1};
			auto pos  = url_args.find('&');
			decltype(pos) old_pos = 0;
			while (pos != std::string::npos)
			{
				auto arg = std::string_view{url_args.data() + old_pos, pos - old_pos};
				auto [key, value] = parse_url_arg(arg);
				result.add_arg(std::move(key), std::move(value));
				old_pos = pos + 1;
				pos = url_args.find('&', old_pos);
			}

			auto last_arg = std::string_view{url_args.data() + old_pos, url_args.length() - old_pos};
			auto [key, value] = parse_url_arg(last_arg);
			result.add_arg(std::move(key), std::move(value));
		}
		else
			url_params_start = resource.length();

		return {std::string{resource.data(), url_params_start}, result};
	}

	friend std::ostream& operator<<(std::ostream& out, const UrlArgs& args)
	{
		if (args._args.empty())
			return out;

		for (std::size_t i = 0; i < args._args.size(); ++i)
		{
			out << (i == 0 ? '?' : '&')
				<< url_encode(args._args[i]->get_name())
				<< '='
				<< url_encode(args._args[i]->get_value());
		}

		return out;
	}

private:
	static std::pair<std::string, std::string> parse_url_arg(const std::string_view& arg)
	{
		auto value_pos = arg.find('=');
		if (value_pos == std::string::npos)
			return { std::string{arg}, std::string{} };

		return {
			url_decode(std::string_view{arg.data(), value_pos}),
			url_decode(std::string_view{arg.data() + value_pos + 1, arg.length() - value_pos - 1})
		};
	}

	std::vector<const UrlArg*> _args;
	std::unordered_map<std::string, UrlArg> _table;
};





class HttpRequest : public HttpMessage
{
public:
	template <typename Method, typename Resource>
	HttpRequest(Method&& method, Resource&& resource)
		: HttpRequest(std::forward<Method>(method), std::forward<Resource>(resource), std::string{}, std::string{}) {}

	template <typename Method, typename Resource, typename Content>
	HttpRequest(Method&& method, Resource&& resource, Content&& content)
		: HttpRequest(std::forward<Method>(method), std::forward<Resource>(resource), std::forward<Content>(content), "text/plain") {}

	template <typename Method, typename Resource, typename Content, typename ContentType>
	HttpRequest(Method&& method, Resource&& resource, Content&& content, ContentType&& content_type)
		: HttpRequest(std::forward<Method>(method), std::forward<Resource>(resource), HttpHeaderTable{}, std::forward<Content>(content), std::forward<ContentType>(content_type)) {}

	template <typename Method, typename Resource, typename Headers, typename Content, typename ContentType>
	HttpRequest(Method&& method, Resource&& resource, Headers&& headers, Content&& content, ContentType&& content_type)
		: HttpRequest(std::forward<Method>(method), std::forward<Resource>(resource), UrlArgs{}, std::forward<Headers>(headers), std::forward<Content>(content), std::forward<ContentType>(content_type))
	{
		auto [new_resource, args] = UrlArgs::parse_from_resource(_resource);
		_resource = std::move(new_resource);
		_args = std::move(args);
	}

	template <typename Method, typename Resource, typename Args, typename Headers, typename Content, typename ContentType>
	HttpRequest(Method&& method, Resource&& resource, Args&& args, Headers&& headers, Content&& content, ContentType&& content_type)
		: HttpMessage(std::forward<Content>(content), std::forward<ContentType>(content_type), std::forward<Headers>(headers))
		, _method(std::forward<Method>(method))
		, _resource(std::forward<Resource>(resource))
		, _args(std::forward<Args>(args))
	{
	}

	HttpRequest(const HttpRequest&) = default;
	HttpRequest(HttpRequest&&) noexcept = default;
	virtual ~HttpRequest() = default;

	HttpRequest& operator=(const HttpRequest&) = default;
	HttpRequest& operator=(HttpRequest&&) noexcept = default;

	const std::string& get_method() const { return _method; }
	const std::string& get_resource() const { return _resource; }
	const UrlArg* get_argument(const std::string& name) const { return _args.get_arg(name); }
	const UrlArgs& get_arguments() const { return _args; }

	bool has_arg(const std::string& name) const { return _args.has_arg(name); }

	virtual std::string dump() const override
	{
		std::ostringstream ss;
		ss << _method << ' ' << _resource << _args << " HTTP/1.1\r\n";
		for (const auto* header : _headers)
			ss << header->get_name() << ": " << header->get_value() << "\r\n";
		ss << "\r\n";
		if (!_content.empty())
			ss << _content;
		return ss.str();
	}

private:
	std::string _method;
	std::string _resource;
	UrlArgs _args;
};




class HttpResponse : public HttpMessage
{
public:
	HttpResponse() : HttpResponse(200) {}
	HttpResponse(int status_code) : HttpResponse(status_code, std::string{}, std::string{}) {}
	HttpResponse(const std::string& content) : HttpResponse(200, content, "text/plain") {}
	HttpResponse(std::string&& content) : HttpResponse(200, std::move(content), "text/plain") {}
	HttpResponse(const nlohmann::json& json) : HttpResponse(200, json) {}
	HttpResponse(int status_code, const nlohmann::json& json) : HttpResponse(status_code, json.dump(), "application/json") {}

	template <typename Content, typename ContentType>
	HttpResponse(int status_code, Content&& content, ContentType&& content_type)
		: HttpResponse(status_code, std::optional<std::string>{}, HttpHeaderTable{}, std::forward<Content>(content), std::forward<ContentType>(content_type)) {}

	template <typename Reason, typename Headers, typename Content, typename ContentType>
	HttpResponse(int status_code, Reason&& reason, Headers&& headers, Content&& content, ContentType&& content_type)
		: HttpMessage(std::forward<Content>(content), std::forward<ContentType>(content_type), std::forward<Headers>(headers))
		, _status_code(status_code)
		, _reason(std::forward<Reason>(reason))
	{
	}

	HttpResponse(const HttpResponse&) = default;
	HttpResponse(HttpResponse&&) noexcept = default;
	virtual ~HttpResponse() = default;

	HttpResponse& operator=(const HttpResponse&) = default;
	HttpResponse& operator=(HttpResponse&&) noexcept = default;

	int get_status_code() const { return _status_code; }

	std::string get_reason() const
	{
		static const std::unordered_map<int, std::string_view> status_code_names = {
			{ 100, "Continue" },
			{ 101, "Switching Protocols" },
			{ 200, "OK" },
			{ 201, "Created" },
			{ 202, "Accepted" },
			{ 203, "Non-Authoritative Information" },
			{ 204, "No Content" },
			{ 205, "Reset Content" },
			{ 206, "Partial Content" },
			{ 300, "Multiple Choices" },
			{ 301, "Moved Permanently" },
			{ 302, "Found" },
			{ 303, "See Other" },
			{ 304, "Not Modified" },
			{ 305, "Use Proxy" },
			{ 307, "Temporary Redirect" },
			{ 400, "Bad Request" },
			{ 401, "Unauthorized" },
			{ 402, "Payment Required" },
			{ 403, "Forbidden" },
			{ 404, "Not Found" },
			{ 405, "Method Not Allowed" },
			{ 406, "Not Acceptable" },
			{ 407, "Proxy Authentication Required" },
			{ 408, "Request Timeout" },
			{ 409, "Conflict" },
			{ 410, "Gone" },
			{ 411, "Length Required" },
			{ 412, "Precondition Failed" },
			{ 413, "Request Entity Too Large" },
			{ 414, "Request-URI Too Long" },
			{ 415, "Unsupported Media Type" },
			{ 416, "Requested Range Not Satisfiable" },
			{ 417, "Expectation Failed" },
			{ 500, "Internal Server Error" },
			{ 501, "Not Implemented" },
			{ 502, "Bad Gateway" },
			{ 503, "Service Unavailable" },
			{ 504, "Gateway Timeout" },
			{ 505, "HTTP Version Not Supported" }
		};

		if (_reason.has_value())
			return _reason.value();
		else
		{
			auto itr = status_code_names.find(_status_code);
			if (itr != status_code_names.end())
				return std::string{itr->second};
		}

		return "Unknown";
	}

	virtual std::string dump() const override
	{
		std::ostringstream ss;
		ss << "HTTP/1.1 " << _status_code << " " << get_reason() << "\r\n";
		for (const auto* header : _headers)
			ss << header->get_name() << ": " << header->get_value() << "\r\n";
		ss << "\r\n";
		if (!_content.empty())
			ss << _content;
		return ss.str();
	}

private:
	int _status_code;
	std::optional<std::string> _reason;
};





class StringStream
{
public:
	StringStream(std::size_t capacity) : _buffer(capacity, 0), _used(0), _read_pos(0) {}
	StringStream(const std::string& str) : _buffer(str.begin(), str.end()), _used(_buffer.size()), _read_pos(0) {}
	StringStream(const StringStream&) = delete;
	StringStream(StringStream&&) noexcept = default;

	StringStream& operator=(const StringStream&) = delete;
	StringStream& operator=(StringStream&&) noexcept = default;

	std::size_t get_capacity() const { return _buffer.size(); }
	std::size_t get_size() const { return _used - _read_pos; }
	std::size_t get_writable_size() const { return get_capacity() - _used; }

	char* get_writable_buffer() { return _buffer.data() + _used; }

	void increase_used(std::size_t count)
	{
		_used = std::min(_used + count, get_capacity());
	}

	std::string_view as_string_view(std::size_t count = 0) const
	{
		return std::string_view{_buffer.data() + _read_pos, count == 0 ? get_size() : std::min(count, get_size())};
	}

	std::optional<char> lookahead_byte() const
	{
		auto sv = as_string_view(1);
		if (sv.empty())
			return std::nullopt;
		return sv[0];
	}

	template <typename T>
	std::size_t lookahead(const T& what) const
	{
		return as_string_view().find(what);
	}

	void skip(std::size_t count)
	{
		count = std::min(count, get_size());
		_read_pos += count;
	}

	std::string_view read(std::size_t count = 0)
	{
		if (count == 0)
			count = get_size();
		else
			count = std::min(count, get_size());
		auto result = std::string_view{_buffer.data() + _read_pos, count};
		_read_pos += count;
		return result;
	}

	std::pair<std::string_view, bool> read_until(char what)
	{
		bool found_delim = true;
		auto pos = lookahead(what);
		if (pos == std::string::npos)
		{
			pos = get_size();
			found_delim = false;
		}
		else if (pos == 0)
			return { std::string_view{}, true };

		return { read(pos), found_delim };
	}

	std::pair<std::string_view, bool> read_until(std::string_view what)
	{
		bool found_delim = true;
		auto pos = lookahead(what);
		if (pos == std::string::npos)
		{
			found_delim = false;

			// Try to find at least the first character and read until then
			// while still reporting that we didn't find delimiter
			pos = lookahead(what[0]);
			if (pos == std::string::npos)
				pos = get_size();
			else // Compare prefix
			{
				auto count = std::min(what.length(), get_size() - pos);
				for (std::size_t i = 1; i < count; ++i)
				{
					// Not a viable prefix so read it whole
					if (what[i] != _buffer[pos + i])
					{
						pos = get_size();
						break;
					}
				}
			}
		}
		else if (pos == 0)
			return { std::string_view{}, true };

		return { read(pos), found_delim };
	}

	template <typename StrT>
	void write_string(const StrT& str)
	{
		std::size_t count = std::min(str.length(), get_writable_size());
		std::memcpy(get_writable_buffer(), str.data(), count);
		increase_used(count);
	}

	void realign()
	{
		if (_read_pos < _used)
			std::memmove(_buffer.data(), _buffer.data() + _read_pos, get_size());
		_used -= _read_pos;
		_read_pos = 0;
	}

private:
	std::vector<char> _buffer;
	std::size_t _used;
	std::size_t _read_pos;
};




namespace detail {

enum class ResponseState
{
	Start,
	StatusLineHttpVersion,
	StatusLineCode,
	StatusLineReason,
	HeaderName,
	HeaderValue,
	Content
};

} // namespace detail

class HttpResponseParser
{
public:
	HttpResponseParser() : _state(detail::ResponseState::Start) {}
	HttpResponseParser(const HttpResponseParser&) = delete;
	HttpResponseParser(HttpResponseParser&&) noexcept = default;

	HttpResponseParser& operator=(const HttpResponseParser&) = delete;
	HttpResponseParser& operator=(HttpResponseParser&&) noexcept = default;

	std::optional<HttpResponse> parse(StringStream& stream)
	{
		bool continue_parsing = true;
		while (continue_parsing)
		{
			switch (_state)
			{
				case detail::ResponseState::Start:
					_http_version.clear();
					_status_code.clear();
					_reason.clear();
					_header_name.clear();
					_header_value.clear();
					_content.clear();
					_headers.clear();
					_content_length = 0;
					_content_type = std::string{};
					_state = detail::ResponseState::StatusLineHttpVersion;
					break;
				case detail::ResponseState::StatusLineHttpVersion:
				{
					auto [str, found_space] = stream.read_until(' ');
					_http_version += str;
					if (found_space)
					{
						_state = detail::ResponseState::StatusLineCode;
						stream.skip(1);
					}
					else
						continue_parsing = false;
					break;
				}
				case detail::ResponseState::StatusLineCode:
				{
					auto [str, found_space] = stream.read_until(' ');
					_status_code += str;
					if (found_space)
					{
						_state = detail::ResponseState::StatusLineReason;
						stream.skip(1);
					}
					else
						continue_parsing = false;
					break;
				}
				case detail::ResponseState::StatusLineReason:
				{
					auto [str, found_newline] = stream.read_until("\r\n");
					_reason += str;
					if (found_newline)
					{
						_state = detail::ResponseState::HeaderName;
						stream.skip(2);
					}
					else
						continue_parsing = false;
					break;
				}
				case detail::ResponseState::HeaderName:
				{
					if (stream.as_string_view(2) == "\r\n")
					{
						stream.skip(2);
						_state = detail::ResponseState::Content;

						auto content_length_header = _headers.get_header("content-length");
						if (content_length_header)
							_content_length = content_length_header->get_value_as<std::uint64_t>();

						auto content_type_header = _headers.get_header("content-type");
						if (content_type_header)
							_content_type = content_type_header->get_value();
					}
					else if (stream.as_string_view(1) == "\r")
					{
						continue_parsing = false;
					}
					else
					{
						auto [str, found_colon] = stream.read_until(':');
						_header_name += str;
						if (found_colon)
						{
							_state = detail::ResponseState::HeaderValue;
							stream.skip(1);
						}
						else
							continue_parsing = false;
					}
					break;
				}
				case detail::ResponseState::HeaderValue:
				{
					auto [str, found_newline] = stream.read_until("\r\n");
					_header_value += str;
					if (found_newline)
					{
						_state = detail::ResponseState::HeaderName;
						stream.skip(2);
						_headers.add_header(std::move(_header_name), lstrip(_header_value));
						_header_name.clear();
						_header_value.clear();
					}
					else
						continue_parsing = false;
					break;
				}
				case detail::ResponseState::Content:
				{
					auto str = stream.read(_content_length - _content.length());
					_content += str;
					if (_content.length() == _content_length)
					{
						_state = detail::ResponseState::Start;
						return HttpResponse{
							std::stoi(_status_code),
							std::move(_reason),
							std::move(_headers),
							std::move(_content),
							std::move(_content_type)
						};
					}
					else
						continue_parsing = false;
					break;
				}
			}
		}

		stream.realign();
		return std::nullopt;
	}

private:
	detail::ResponseState _state;
	std::string _http_version, _status_code, _reason, _header_name, _header_value, _content;
	HttpHeaderTable _headers;
	std::uint64_t _content_length;
	std::string _content_type;
};









struct Network
{
	static ssize_t read(int fd, void* buf, size_t len)
	{
		return ::recv(fd, buf, len, 0);
	}

	static ssize_t write(int fd, const void* buf, size_t len)
	{
		return ::send(fd, buf, len, 0);
	}
};

struct NonNetwork
{
	static ssize_t read(int fd, void* buf, size_t len)
	{
		return ::read(fd, buf, len);
	}

	static ssize_t write(int fd, const void* buf, size_t len)
	{
		return ::write(fd, buf, len);
	}
};

class SocketError : public std::exception
{
public:
	SocketError(const char* msg) noexcept : _msg(msg) {}

	virtual const char* what() const noexcept { return _msg; }

private:
	const char* _msg;
};

template <typename SocketOp = Network>
class Socket
{
public:
	Socket() : Socket(::socket(AF_UNIX, SOCK_STREAM, 0)) {}

	Socket(int fd) : _fd(fd), _stream(4096)
	{
		if (_fd < 0)
			throw SocketError("Unable to create socket");

		if (::fcntl(_fd, F_SETFL, O_NONBLOCK) < 0)
			throw SocketError("Unable to switch socket to non-blocking mode");
	}

	~Socket()
	{
		close();
	}

	Socket(Socket&& rhs) noexcept : _fd(rhs._fd), _stream(std::move(rhs._stream))
	{
		rhs._fd = 0;
	}

	Socket& operator=(Socket&& rhs) noexcept
	{
		_fd = rhs._fd;
		_stream = std::move(rhs._stream);
		rhs._fd = 0;
		return *this;
	}

	Socket(const Socket&) = delete;
	Socket& operator=(const Socket&) = delete;

	int get_fd() const { return _fd; }
	StringStream& get_stream() { return _stream; }
	pollfd get_poll_fd() const { return {_fd, POLLIN, 0}; }

	bool is_closed() const { return _fd == 0; }

	bool is_listening() const
	{
		int result;
		socklen_t len = sizeof(result);
		if (::getsockopt(_fd, SOL_SOCKET, SO_ACCEPTCONN, &result, &len) == -1)
			return false;
		return result > 0;
	}

	void connect(const std::string& file_path)
	{
		auto sa = create_sockaddr(file_path);
		if (::connect(_fd, reinterpret_cast<sockaddr*>(&sa), SUN_LEN(&sa)) < 0)
			throw SocketError("Error while connecting to the local socket");
	}

	void listen(const std::string& file_path)
	{
		auto sa = create_sockaddr(file_path);
		if (::bind(_fd, reinterpret_cast<sockaddr*>(&sa), SUN_LEN(&sa)) < 0)
			throw SocketError("Unable to bind the local socket");

		if (::listen(_fd, 16) < 0)
			throw SocketError("Unable to start listening to the local socket");
	}

	std::optional<Socket> accept_connection()
	{
		auto client_fd = ::accept(_fd, nullptr, nullptr);
		if (client_fd < 0)
		{
			if (errno == EWOULDBLOCK)
				return std::nullopt;

			throw SocketError("Error while accepting new connection on the local socket");
		}

		return client_fd;
	}

	void read()
	{
		int n = 0;

		do
		{
			n = SocketOp::read(_fd, _stream.get_writable_buffer(), _stream.get_writable_size());
			if (n < 0)
			{
				if (errno == EWOULDBLOCK)
					return;

				throw SocketError("Error while reading data from the local socket");
			}

			_stream.increase_used(n);
		}
		while (n > 0);
	}

	void write(const std::string& str)
	{
		std::size_t sent = 0;
		while (sent < str.length())
		{
			auto n = SocketOp::write(_fd, str.data() + sent, str.length() - sent);
			if (n < 0)
			{
				if (errno == EWOULDBLOCK)
					return;

				throw SocketError("Error while writing data to the local socket");
			}

			sent += static_cast<std::size_t>(n);
		}
	}

	void close()
	{
		if (_fd != 0)
		{
			::shutdown(_fd, SHUT_RDWR);
			::close(_fd);
			_fd = 0;
		}
	}

private:
	sockaddr_un create_sockaddr(const std::string& file_path)
	{
		sockaddr_un sa;
		memset(&sa, 0, sizeof(sockaddr_un));

		sa.sun_family = AF_UNIX;
		std::strncpy(sa.sun_path, file_path.c_str(), std::min(file_path.length(), sizeof(sa.sun_path) - 1));

		return sa;
	}

	int _fd;
	StringStream _stream;
};

#define ULOCAL_VERSION "0.3.0"










class RequestError : public std::exception
{
public:
	RequestError(const char* msg) noexcept : _msg(msg) {}

	virtual const char* what() const noexcept { return _msg; }

private:
	const char* _msg;
};

class HttpClient
{
public:
	HttpClient(const std::string& local_socket_path, const std::string& host_header = "ulocal " ULOCAL_VERSION) : _local_socket_path(local_socket_path), _host_header(host_header) {}

	template <typename Method, typename Resource>
	HttpResponse send_request(Method&& method, Resource&& resource)
	{
		return send_request(
			std::forward<Method>(method),
			std::forward<Resource>(resource),
			std::string{},
			std::string{},
			HttpHeaderTable{}
		);
	}

	template <typename Method, typename Resource>
	HttpResponse send_request(Method&& method, Resource&& resource, const std::string& content)
	{
		return send_request(
			std::forward<Method>(method),
			std::forward<Resource>(resource),
			content,
			"text/plain",
			HttpHeaderTable{}
		);
	}

	template <typename Method, typename Resource>
	HttpResponse send_request(Method&& method, Resource&& resource, std::string&& content)
	{
		return send_request(
			std::forward<Method>(method),
			std::forward<Resource>(resource),
			std::move(content),
			"text/plain",
			HttpHeaderTable{}
		);
	}

	template <typename Method, typename Resource>
	HttpResponse send_request(Method&& method, Resource&& resource, const nlohmann::json& content)
	{
		return send_request(
			std::forward<Method>(method),
			std::forward<Resource>(resource),
			content.dump(),
			"application/json",
			HttpHeaderTable{}
		);
	}

	template <typename Method, typename Resource, typename Content, typename ContentType, typename Headers>
	HttpResponse send_request(Method&& method, Resource&& resource, Content&& content, ContentType&& content_type, Headers headers)
	{
		if (!headers.has_header("Host"))
			headers.add_header("Host", _host_header);

		HttpRequest request{
			std::forward<Method>(method),
			std::forward<Resource>(resource),
			std::move(headers),
			std::forward<Content>(content),
			std::forward<ContentType>(content_type)
		};
		request.calculate_content_length();

		Socket<> socket;
		socket.connect(_local_socket_path);
		socket.write(request.dump());

		HttpResponseParser response_parser;
		std::optional<HttpResponse> maybe_response;
		auto pollfd = socket.get_poll_fd();
		bool still_poll = true;

		while (still_poll && !maybe_response)
		{
			auto result = ::poll(&pollfd, 1, -1);
			if (result == -1)
				throw RequestError("Unable to obtain response from the server");

			if (pollfd.revents & POLLIN)
			{
				socket.read();
				maybe_response = response_parser.parse(socket.get_stream());
			}

			if (pollfd.revents & POLLHUP)
				still_poll = false;
		}

		if (!maybe_response)
			throw RequestError("Server closed connection unexpectedly");

		return std::move(maybe_response).value();
	}

private:
	std::string _local_socket_path, _host_header;
};






namespace detail {

enum class RequestState
{
	Start,
	StatusLineMethod,
	StatusLineResource,
	StatusLineHttpVersion,
	HeaderName,
	HeaderValue,
	Content
};

} // namespace detail

class HttpRequestParser
{
public:
	HttpRequestParser() : _state(detail::RequestState::Start) {}
	HttpRequestParser(const HttpRequestParser&) = delete;
	HttpRequestParser(HttpRequestParser&&) noexcept = default;

	HttpRequestParser& operator=(const HttpRequestParser&) = delete;
	HttpRequestParser& operator=(HttpRequestParser&&) noexcept = default;

	std::optional<HttpRequest> parse(StringStream& stream)
	{
		bool continue_parsing = true;
		while (continue_parsing)
		{
			switch (_state)
			{
				case detail::RequestState::Start:
				{
					_method.clear();
					_resource.clear();
					_http_version.clear();
					_header_name.clear();
					_header_value.clear();
					_headers.clear();
					_content.clear();
					_content_length = 0;
					_content_type = std::string{};
					_state = detail::RequestState::StatusLineMethod;
					break;
				}
				case detail::RequestState::StatusLineMethod:
				{
					auto [str, found_space] = stream.read_until(' ');
					_method += str;
					if (found_space)
					{
						_state = detail::RequestState::StatusLineResource;
						stream.skip(1);
					}
					else
						continue_parsing = false;
					break;
				}
				case detail::RequestState::StatusLineResource:
				{
					auto [str, found_space] = stream.read_until(' ');
					_resource += str;
					if (found_space)
					{
						_state = detail::RequestState::StatusLineHttpVersion;
						stream.skip(1);
					}
					else
						continue_parsing = false;
					break;
				}
				case detail::RequestState::StatusLineHttpVersion:
				{
					auto [str, found_newline] = stream.read_until("\r\n");
					_http_version += str;
					if (found_newline)
					{
						_state = detail::RequestState::HeaderName;
						stream.skip(2);
					}
					else
						continue_parsing = false;
					break;
				}
				case detail::RequestState::HeaderName:
				{
					if (stream.as_string_view(2) == "\r\n")
					{
						stream.skip(2);
						_state = detail::RequestState::Content;

						auto content_length_header = _headers.get_header("content-length");
						if (content_length_header)
							_content_length = content_length_header->get_value_as<std::uint64_t>();

						auto content_type_header = _headers.get_header("content-type");
						if (content_type_header)
							_content_type = content_type_header->get_value();
					}
					else if (stream.as_string_view(1) == "\r")
					{
						continue_parsing = false;
					}
					else
					{
						auto [str, found_colon] = stream.read_until(':');
						_header_name += str;
						if (found_colon)
						{
							_state = detail::RequestState::HeaderValue;
							stream.skip(1);
						}
						else
							continue_parsing = false;
					}
					break;
				}
				case detail::RequestState::HeaderValue:
				{
					auto [str, found_newline] = stream.read_until("\r\n");
					_header_value += str;
					if (found_newline)
					{
						_state = detail::RequestState::HeaderName;
						stream.skip(2);
						_headers.add_header(std::move(_header_name), lstrip(_header_value));
						_header_name.clear();
						_header_value.clear();
					}
					else
						continue_parsing = false;
					break;
				}
				case detail::RequestState::Content:
				{
					auto str = stream.read(_content_length - _content.length());
					_content += str;
					if (_content.length() == _content_length)
					{
						_state = detail::RequestState::Start;
						return HttpRequest{
							std::move(_method),
							std::move(_resource),
							std::move(_headers),
							std::move(_content),
							std::move(_content_type)
						};
					}
					else
						continue_parsing = false;
					break;
				}
			}
		}

		stream.realign();
		return std::nullopt;
	}

private:
	detail::RequestState _state;
	std::string _method, _resource, _http_version, _header_name, _header_value, _content;
	HttpHeaderTable _headers;
	std::uint64_t _content_length;
	std::string _content_type;
};



class HttpConnection
{
public:
	HttpConnection(Socket<>&& socket) : _socket(std::move(socket)), _request_parser() {}
	HttpConnection(const HttpConnection&) = delete;
	HttpConnection(HttpConnection&&) noexcept = default;

	HttpConnection& operator=(const HttpConnection&) = delete;
	HttpConnection& operator=(HttpConnection&&) noexcept = default;

	Socket<>& get_socket() { return _socket; }
	const Socket<>& get_socket() const { return _socket; }
	std::optional<HttpRequest> get_request() { return _request_parser.parse(_socket.get_stream()); }

private:
	Socket<> _socket;
	HttpRequestParser _request_parser;
};





class Pipe
{
public:
	Pipe() : _read_socket(), _write_socket()
	{
		int fds[2];
		if (::pipe(fds) != 0)
			throw std::runtime_error("Unable to create pipe");

		_read_socket = std::make_unique<Socket<NonNetwork>>(fds[0]);
		_write_socket = std::make_unique<Socket<NonNetwork>>(fds[1]);
	}

	Pipe(const Pipe&) = delete;
	Pipe(Pipe&&) noexcept = default;
	~Pipe() = default;

	Pipe& operator=(const Pipe&) = delete;
	Pipe& operator=(Pipe&& o) noexcept
	{
		std::swap(_read_socket, o._read_socket);
		std::swap(_write_socket, o._write_socket);
		return *this;
	}

	int get_read_fd() const { return _read_socket->get_fd(); }
	int get_write_fd() const { return _write_socket->get_fd(); }

	Socket<NonNetwork>* get_read_socket() { return _read_socket.get(); }
	Socket<NonNetwork>* get_write_socket() { return _write_socket.get(); }

private:
	std::unique_ptr<Socket<NonNetwork>> _read_socket;
	std::unique_ptr<Socket<NonNetwork>> _write_socket;
};




template <typename Callback>
class Endpoint
{
public:
	template <typename R, typename M, typename C>
	Endpoint(R&& route, M&& methods, C&& callback)
		: _route(std::forward<R>(route)), _methods(std::forward<M>(methods)), _callback(std::forward<C>(callback)) {}

	const std::string& get_route() const { return _route; }

	bool supports_method(const std::string& method) const
	{
		return _methods.find(method) != _methods.end();
	}

	template <typename... Args>
	auto perform(Args&&... args) const
	{
		return _callback(std::forward<Args>(args)...);
	}

private:
	std::string _route;
	std::unordered_set<std::string, CaseInsensitiveHash, CaseInsensitiveCompare> _methods;
	Callback _callback;
};




template <typename Callback>
class RouteTable
{
public:
	RouteTable() : _table() {}

	bool has_route(const std::string& route) const
	{
		return _table.find(route) != _table.end();
	}

	bool has_route_for_method(const std::string& route, const std::string& method) const
	{
		auto route_itr = _table.find(route);
		if (route_itr == _table.end())
			return false;

		return route_itr->second.find(method) != route_itr->second.end();
	}

	template <typename R, typename M, typename C>
	void add_route(R&& route, M&& methods, C&& callback)
	{
		auto itr = _table.find(route);
		if (itr == _table.end())
			std::tie(itr, std::ignore) = _table.emplace(route, MethodTable{});

		auto& method_table = itr->second;
		for (const auto& method : methods)
		{
			auto method_itr = method_table.find(method);
			if (method_itr == method_table.end())
				method_table.emplace(method, callback);
			else
				method_itr->second = callback;
		}
	}

	template <typename... Args>
	auto perform_action(const std::string& route, const std::string& method, Args&&... args) const
	{
		return _table.at(route).at(method)(std::forward<Args>(args)...);
	}

private:
	using MethodTable = std::unordered_map<std::string, Callback, CaseInsensitiveHash, CaseInsensitiveCompare>;

	std::unordered_map<std::string, MethodTable, CaseInsensitiveHash, CaseInsensitiveCompare> _table;
};












class HttpServer
{
public:
	using RequestCallback = std::function<HttpResponse(const HttpRequest&)>;

	HttpServer(const std::string& local_socket_path)
		: _routes(), _local_socket_path(local_socket_path), _server(), _clients(), _thread(), _control_pipe(), _server_header() {}
	HttpServer(const std::string& local_socket_path, const std::string& server_header) : HttpServer(local_socket_path)
	{
		_server_header = server_header;
	}

	template <typename Fn>
	void endpoint(const std::initializer_list<std::string>& methods, const std::string& route, const Fn& fn)
	{
		_routes.add_route(route, methods, fn);
	}

	bool is_serving() const
	{
		return _server.is_listening();
	}

	void serve()
	{
		_server.listen(_local_socket_path);

		_thread = std::thread([this]() {
			bool running = true;
			while (running)
			{
				std::vector<pollfd> poll_fds;
				poll_fds.reserve(_clients.size() + 1);
				for (const auto& connection : _clients)
					poll_fds.push_back(connection.get_socket().get_poll_fd());
				poll_fds.push_back(_server.get_poll_fd());
				poll_fds.push_back(_control_pipe.get_read_socket()->get_poll_fd());

				auto* server_pollfd = &poll_fds[poll_fds.size() - 2];
				auto* control_pipe_pollfd = &poll_fds[poll_fds.size() - 1];

				auto result = ::poll(poll_fds.data(), poll_fds.size(), -1);
				if (result == -1)
					throw SocketError("Failed while polling HTTP connections");
				else if (result == 0)
					continue;

				if (control_pipe_pollfd->revents & POLLIN)
				{
					_control_pipe.get_read_socket()->read();
					auto command = _control_pipe.get_read_socket()->get_stream().read_until('\0');
					if (command.first == "stop")
						running = false;
				}

				if (server_pollfd->revents & POLLIN)
				{
					auto new_client = _server.accept_connection();
					while (new_client)
					{
						_clients.push_back(std::move(new_client).value());
						new_client = _server.accept_connection();
					}
				}

				std::size_t index = 0;
				for (auto& connection : _clients)
				{
					if (index >= poll_fds.size() - 2)
						break;

					auto& poll_fd = poll_fds[index];
					if (poll_fd.revents & POLLIN)
					{
						std::optional<HttpResponse> response;

						try
						{
							connection.get_socket().read();
						}
						catch (const std::exception& err)
						{
							response = HttpResponse{500, err.what()};
						}

						auto maybe_request = connection.get_request();
						if (maybe_request)
						{
							auto request = std::move(maybe_request).value();
							if (!_routes.has_route(request.get_resource()))
								response = HttpResponse{404};
							else if (!_routes.has_route_for_method(request.get_resource(), request.get_method()))
								response = HttpResponse{405};
							else
							{
								try
								{
									response = _routes.perform_action(request.get_resource(), request.get_method(), request);
								}
								catch (const std::exception& err)
								{
									response = HttpResponse{500, err.what()};
								}
							}
						}

						if (response)
						{
							response->calculate_content_length();
							if (_server_header)
								response->add_header("Server", _server_header.value());
							response->add_header("Connection", "close");
							response->add_header("X-Framework", "ulocal " ULOCAL_VERSION);

							try
							{
								connection.get_socket().write(response->dump());
							}
							catch (const std::exception& err)
							{
								;
							}
							connection.get_socket().close();
						}
					}

					if (poll_fd.revents & POLLHUP)
						connection.get_socket().close();

					++index;
				}

				auto remove_itr = std::remove_if(_clients.begin(), _clients.end(), [](const auto& connection) {
					return connection.get_socket().is_closed();
				});
				_clients.erase(remove_itr, _clients.end());
			}
		});
	}

	void wait_until_done()
	{
		_thread.join();
	}

	void terminate()
	{
		_control_pipe.get_write_socket()->write("stop\0");
	}

private:
	RouteTable<RequestCallback> _routes;
	std::string _local_socket_path;
	Socket<> _server;
	std::vector<HttpConnection> _clients;

	std::thread _thread;
	Pipe _control_pipe;

	std::optional<std::string> _server_header;
};





} // namespace ulocal

