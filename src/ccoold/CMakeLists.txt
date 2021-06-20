set(SOURCES
	buffer.cpp
	ccool_daemon.cpp
	device_detector.cpp
	devices/all.cpp
	interfaces/interface.cpp
	interfaces/debug/debug_interface.cpp
	interfaces/debug/debug_device_interface.cpp
	interfaces/usb/usb_interface.cpp
	interfaces/usb/usb_device_interface.cpp
)

add_library(libccoold STATIC ${SOURCES})
target_include_directories(libccoold PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(libccoold PUBLIC ccool_common spdlog::spdlog LibUSB::LibUSB json ulocal)
set_target_properties(libccoold PROPERTIES OUTPUT_NAME "ccoold")

add_executable(ccoold ccoold.cpp)
target_link_libraries(ccoold PUBLIC libccoold cxxopts)
