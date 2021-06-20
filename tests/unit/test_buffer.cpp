#include <string_view>

#include <catch2/catch.hpp>

#include "buffer.hpp"

using namespace ccool;
using namespace std::literals;

TEST_CASE("Buffer tests", "utils") {
	SECTION("empty buffer") {
		Buffer buffer;
		CHECK(buffer.get_data() == BytesView{});
		CHECK(buffer.get_size() == 0);
	}

	SECTION("create from hex string") {
		Buffer buffer{"0123456789abcdef"};
		CHECK(buffer.get_size() == 8);
	}

	SECTION("write int8") {
		Buffer buffer;
		buffer.write<Endian::Little, std::int8_t>(0x12);
		CHECK(buffer.get_data() == "\x12"_bv);
		CHECK(buffer.get_size() == 1);
	}

	SECTION("write int16 (little endian)") {
		Buffer buffer;
		buffer.write<Endian::Little, std::int16_t>(0x3456);
		CHECK(buffer.get_data() == "\x56\x34"_bv);
		CHECK(buffer.get_size() == 2);
	}

	SECTION("write int16 (big endian)") {
		Buffer buffer;
		buffer.write<Endian::Big, std::int16_t>(0x3456);
		CHECK(buffer.get_data() == "\x34\x56"_bv);
		CHECK(buffer.get_size() == 2);
	}

	SECTION("write int32 (little endian)") {
		Buffer buffer;
		buffer.write<Endian::Little, std::int32_t>(0x789ABCDE);
		CHECK(buffer.get_data() == "\xDE\xBC\x9A\x78"_bv);
		CHECK(buffer.get_size() == 4);
	}

	SECTION("write int32 (big endian)") {
		Buffer buffer;
		buffer.write<Endian::Big, std::int32_t>(0x789ABCDE);
		CHECK(buffer.get_data() == "\x78\x9A\xBC\xDE"_bv);
		CHECK(buffer.get_size() == 4);
	}

	SECTION("write fixed point 16-bit") {
		Buffer buffer;
		buffer.write<Endian::Little, FixedPoint<16>>(FixedPoint<16>{4.5});
		CHECK(buffer.get_data() == "\x05\x04"_bv);
		CHECK(buffer.get_size() == 2);
	}

	SECTION("write array of multiple int8") {
		Buffer buffer;
		buffer.write<Endian::Little, std::vector<std::int8_t>>({0x01, 0x02, 0x03, 0x04});
		CHECK(buffer.get_data() == "\x01\x02\x03\x04"_bv);
		CHECK(buffer.get_size() == 4);
	}

	SECTION("read int8") {
		Buffer buffer{"\x12"_bv};
		REQUIRE(buffer.get_size() == 1);
		CHECK(buffer.read<Endian::Little, std::int8_t>() == 0x12);
	}

	SECTION("read int16 (little endian)") {
		Buffer buffer{"\x12\x34"_bv};
		REQUIRE(buffer.get_size() == 2);
		CHECK(buffer.read<Endian::Little, std::int16_t>() == 0x3412);
	}

	SECTION("read int16 (big endian)") {
		Buffer buffer{"\x12\x34"_bv};
		REQUIRE(buffer.get_size() == 2);
		CHECK(buffer.read<Endian::Big, std::int16_t>() == 0x1234);
	}

	SECTION("read fixed point 16-bit") {
		Buffer buffer{"\x02\x04"_bv};
		REQUIRE(buffer.get_size() == 2);
		CHECK(buffer.read<Endian::Little, FixedPoint<16>>() == FixedPoint<16>{4.2});
	}

	SECTION("read array of multiple int8") {
		Buffer buffer{"\x01\x02\x03\x04"_bv};
		REQUIRE(buffer.get_size() == 4);
		CHECK(buffer.read<Endian::Little, std::vector<std::int8_t>>(4) == std::vector<std::int8_t>{0x01, 0x02, 0x03, 0x04});
	}

	SECTION("consecutive reads") {
		Buffer buffer{"\x12\x34\x11\x22"_bv};
		REQUIRE(buffer.get_size() == 4);
		CHECK(buffer.read<Endian::Big, std::uint16_t>() == 0x1234);
		CHECK(buffer.read<Endian::Big, std::uint16_t>() == 0x1122);
	}
}
