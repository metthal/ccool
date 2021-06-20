#pragma once

#include <unordered_map>

#include <libusb-1.0/libusb.h>

#include <interfaces/device_interface.hpp>

namespace ccool {

class UsbDeviceInterface : public DeviceInterface
{
public:
	UsbDeviceInterface(libusb_device* device);
	virtual ~UsbDeviceInterface();

	virtual void bind() override;

	virtual std::uint32_t get_vendor_id() override;
	virtual std::uint32_t get_product_id() override;

	virtual void control(std::uint32_t request_type, std::uint32_t request, std::uint32_t value) override;
	virtual void send(std::uint8_t endpoint, const Buffer& data) override;
	virtual Buffer recv(std::uint8_t endpoint) override;

private:
	std::size_t get_endpoint_mtu(std::uint8_t endpoint);

	libusb_device* _device;
	libusb_device_descriptor _device_desc;
	libusb_device_handle* _handle;
	bool _reattach_kernel_driver;
	bool _release_interface;
	std::unordered_map<std::uint8_t, std::size_t> _endpoint_mtu;
};

} // namespace ccool
