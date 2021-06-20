#pragma once

#include <string_view>
#include <unordered_map>
#include <vector>

#include "conversion.hpp"

namespace ccool {

enum class ArgumentType
{
	Option,
	Toggle
};

class ArgParser
{
	struct Option
	{
		std::uint32_t id;
		std::string_view long_option;
		char short_option;
	};

	class Result
	{
	public:
		template <typename T>
		std::optional<T> get(std::string_view long_arg) const
		{
			auto opt_itr = parent->_long_options.find(long_arg);
			if (opt_itr == parent->_long_options.end())
				return nullptr;

			auto arg_itr = _arguments.find(opt_itr->second->id);
			if (arg_itr == _arguments.end())
				return std::nullopt;

			return convert<T>(arg_itr->second);
		}

		template <typename T>
		std::optional<T> get(char arg) const
		{
			auto opt_itr = parent->_short_options.find(arg);
			if (opt_itr == parent->_short_options.end())
				return std::nullopt;

			auto arg_itr = _arguments.find(opt_itr->second->id);
			if (arg_itr == _arguments.end())
				return std::nullopt;

			return convert<T>(arg_itr->second);
		}

	private:
		const ArgParser* parent;
		std::unordered_map<std::uint32_t, std::string_view> _arguments;
	};

public:
	ArgParser();

	void flag(std::string_view arg, bool default_flag, std::string_view description);
	void flag(char short_arg, bool default_flag, std::string_view description);
	void flag(std::string_view arg, char short_arg, bool default_flag, std::string_view description);
	void option(std::string_view arg, std::string_view description);
	void option(char short_arg, std::string_view description);
	void option(std::string_view arg, char short_arg, std::string_view description);

	std::string help() const;
	Result parse(int& argc, char**& argv) const;

private:
	std::vector<Option> _options;
	std::unordered_map<char, const Option*> _short_options;
	std::unordered_map<std::string_view, const Option*> _long_options;
};

} // namespace ccool
