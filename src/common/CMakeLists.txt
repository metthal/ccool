set(SOURCES
	common.cpp
	string.cpp
)

add_library(ccool_common STATIC ${SOURCES})
target_compile_definitions(ccool_common PUBLIC -DFMT_HEADER_ONLY)
target_include_directories(ccool_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
