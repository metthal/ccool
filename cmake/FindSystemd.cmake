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
	pkg_check_modules(PKG_SYSTEMD libsystemd)
endif()

find_path(
	SYSTEMD_INCLUDE_DIR
	NAMES
		sd-journal.h
	HINTS ${PKG_SYSTEMD_INCLUDEDIR}
	PATH_SUFFIXES
		include/systemd
)

find_library(
	SYSTEMD_LIBRARIES
	NAMES systemd
	HINTS ${PKG_SYSTEMD_LIBDIR}
	PATHS
		/usr/lib64
		/usr/lib
)

mark_as_advanced(SYSTEMD_INCLUDE_DIR SYSTEMD_LIBRARIES)

if(SYSTEMD_INCLUDE_DIR AND SYSTEMD_LIBRARIES)
	set(SYSTEMD_FOUND 1)

	if(NOT TARGET Systemd::Systemd)
		add_library(Systemd::Systemd UNKNOWN IMPORTED)
		set_target_properties(Systemd::Systemd PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${SYSTEMD_INCLUDE_DIR}")
		set_target_properties(Systemd::Systemd PROPERTIES IMPORTED_LOCATION "${SYSTEMD_LIBRARIES}")
	endif()
endif()

find_package_handle_standard_args(
	Systemd
	REQUIRED_VARS
		SYSTEMD_LIBRARIES
		SYSTEMD_INCLUDE_DIR
)
