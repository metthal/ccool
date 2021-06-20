#include <interfaces/usb/usb_interface.hpp>
#include <interfaces/usb/usb_device_interface.hpp>
#include <scope_exit.hpp>

namespace ccool {

UsbInterface::UsbInterface()
{
	libusb_init(&_context);
}

UsbInterface::~UsbInterface()
{
	if (_context)
	{
		libusb_exit(_context);
		_context = nullptr;
	}
}

std::vector<std::unique_ptr<DeviceInterface>> UsbInterface::get_device_interfaces()
{
	libusb_device** device_list;
	auto device_count = libusb_get_device_list(_context, &device_list);
	if (device_count < 0)
		return {};

	on_scope_exit([&]() {
		libusb_free_device_list(device_list, 1);
	});

	std::vector<std::unique_ptr<DeviceInterface>> result;
	for (decltype(device_count) i = 0; i < device_count; ++i)
		result.emplace_back(std::make_unique<UsbDeviceInterface>(device_list[i]));

	return result;
}

} // namespace ccool
