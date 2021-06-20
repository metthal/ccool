#include <string_view>
#include <vector>

#include <catch2/catch.hpp>

#include "string.hpp"

using namespace ccool;

TEST_CASE("String tests", "split") {
	CHECK(split("", ',') == std::vector<std::string_view>{""});
	CHECK(split(",", ',') == std::vector<std::string_view>{"", ""});
	CHECK(split("a,b", ',') == std::vector<std::string_view>{"a", "b"});
	CHECK(split("ab,cde", ',') == std::vector<std::string_view>{"ab", "cde"});
	CHECK(split(",cde", ',') == std::vector<std::string_view>{"", "cde"});
	CHECK(split("ab,", ',') == std::vector<std::string_view>{"ab", ""});
	CHECK(split("ab,,,", ',') == std::vector<std::string_view>{"ab", "", "", ""});
	CHECK(split("abcd", ',') == std::vector<std::string_view>{"abcd"});
}
