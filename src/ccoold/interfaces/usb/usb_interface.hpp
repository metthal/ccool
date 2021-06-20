#pragma once

#include <libusb-1.0/libusb.h>

#include <interfaces/device_interface.hpp>
#include <interfaces/interface.hpp>

namespace ccool {

class UsbInterface : public Interface
{
public:
	UsbInterface();
	virtual ~UsbInterface();

	virtual std::vector<std::unique_ptr<DeviceInterface>> get_device_interfaces() override;

private:
	libusb_context* _context;
};

} // namespace ccool
