find_package(Catch2 REQUIRED CONFIG)

set(SOURCES
	unit_tests.cpp
	test_buffer.cpp
	test_conversion.cpp
	test_string.cpp
)

add_executable(unit_tests ${SOURCES})
target_link_libraries(unit_tests PRIVATE ccool_common libccoold Catch2::Catch2)
