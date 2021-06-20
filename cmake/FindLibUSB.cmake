# Finder for LibUSB library.
#
# Imported targets:
# LibUSB::LibUSB
#
# Result variables:
# LIBUSB_FOUND - System has found LibUSB library.
# LIBUSB_INCLUDE_DIR - LibUSB include directory.
# LIBUSB_LIBRARIES - LibUSB library files.
# LIBUSB_VERSION - Version of LibUSB library.

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
	pkg_check_modules(PKG_LIBUSB QUIET libusb-1.0)
endif()

find_path(
	LIBUSB_INCLUDE_DIR
	NAMES
		libusb.h
	HINTS ${PKG_LIBUSB_INCLUDEDIR}
	PATH_SUFFIXES
		include/libusb-1.0
)

find_library(
	LIBUSB_LIBRARIES
	NAMES usb-1.0
	HINTS ${PKG_LIBUSB_LIBDIR}
	PATHS
		/usr/lib64
		/usr/lib
)

mark_as_advanced(LIBUSB_INCLUDE_DIR LIBUSB_LIBRARIES)

if(LIBUSB_INCLUDE_DIR AND LIBUSB_LIBRARIES)
	set(LIBUSB_FOUND 1)

	file(STRINGS "${LIBUSB_INCLUDE_DIR}/libusb.h" LIBUSB_VERSION_DEFINE REGEX "^#[ \t]*define[ \t]+LIBUSB_API_VERSION[ \t]+0x[0-9]+.*")
	string(REGEX REPLACE "^#[ \t]*define[ \t]+LIBUSB_API_VERSION[ \t]+0x([0-9a-f][0-9a-f])([0-9a-f][0-9a-f])01([0-9a-f][0-9a-f]).*" "\\1;\\2;\\3" LIBUSB_VERSION_LIST "${LIBUSB_VERSION_DEFINE}")
	list(GET LIBUSB_VERSION_LIST 0 LIBUSB_VERSION_MAJOR_HEX)
	list(GET LIBUSB_VERSION_LIST 1 LIBUSB_VERSION_MINOR_HEX)
	list(GET LIBUSB_VERSION_LIST 2 LIBUSB_VERSION_PATCH_HEX)

	math(EXPR LIBUSB_VERSION_MAJOR ${LIBUSB_VERSION_MAJOR_HEX})
	math(EXPR LIBUSB_VERSION_MINOR ${LIBUSB_VERSION_MINOR_HEX})
	math(EXPR LIBUSB_VERSION_PATCH "16 + ${LIBUSB_VERSION_PATCH_HEX}")

	set(LIBUSB_VERSION "${LIBUSB_VERSION_MAJOR}.${LIBUSB_VERSION_MINOR}.${LIBUSB_VERSION_PATCH}")

	if(NOT TARGET LibUSB::LibUSB)
		add_library(LibUSB::LibUSB UNKNOWN IMPORTED)
		set_target_properties(LibUSB::LibUSB PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${LIBUSB_INCLUDE_DIR}")
		set_target_properties(LibUSB::LibUSB PROPERTIES IMPORTED_LOCATION "${LIBUSB_LIBRARIES}")
	endif()
endif()

find_package_handle_standard_args(
	LibUSB
	REQUIRED_VARS
		LIBUSB_LIBRARIES
		LIBUSB_INCLUDE_DIR
	VERSION_VAR
		LIBUSB_VERSION
)