set(SOURCES
	libccool.cpp
)

add_library(libccool STATIC ${SOURCES})
target_link_libraries(libccool PUBLIC ccool_common json ulocal)

add_executable(ccool ccool.cpp)
target_link_libraries(ccool PUBLIC libccool cxxopts)
