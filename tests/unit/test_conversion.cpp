#include <catch2/catch.hpp>

#include "conversion.hpp"

using namespace ccool;
using namespace std::literals;

TEST_CASE("Conversion tests", "utils") {
	CHECK(convert<long>(123) == 123);
	CHECK(convert<long>(123) == 123);
	CHECK(convert<int>("123") == 123);
	CHECK(convert<int>("123"s) == 123);
	CHECK(convert<int>("123"sv) == 123);
	CHECK(convert<int>("-123") == -123);
	CHECK(convert<int>("-123"s) == -123);
	CHECK(convert<int>("-123"sv) == -123);
	CHECK(convert<bool>("0") == false);
	CHECK(convert<bool>("1") == true);
	CHECK(convert<bool>("true") == true);
	CHECK(convert<bool>("false") == false);
	CHECK(convert<bool>("true"sv) == true);
	CHECK(convert<bool>("false"sv) == false);
	CHECK(convert<bool>("true"s) == true);
	CHECK(convert<bool>("false"s) == false);
	CHECK(convert<std::string>(123) == "123");
	CHECK(convert<std::string>(-123) == "-123");
	CHECK(convert<std::string_view>("123") == "123");
	CHECK(convert<std::string_view>("-123") == "-123");
	CHECK(convert<std::string>(true) == "true");
	CHECK(convert<std::string>(false) == "false");
}
