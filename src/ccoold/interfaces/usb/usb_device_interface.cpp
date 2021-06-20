#include <interfaces/usb/usb_device_interface.hpp>

namespace ccool {

UsbDeviceInterface::UsbDeviceInterface(libusb_device* device) : _device(device), _device_desc(), _handle(nullptr),
	_reattach_kernel_driver(false), _release_interface(false), _endpoint_mtu()
{
	libusb_get_device_descriptor(_device, &_device_desc);
}

UsbDeviceInterface::~UsbDeviceInterface()
{
	if (_handle)
	{
		if (_release_interface)
			libusb_release_interface(_handle, 0);
		if (_reattach_kernel_driver)
			libusb_attach_kernel_driver(_handle, 0);
		libusb_close(_handle);
		_handle = nullptr;
	}
}

void UsbDeviceInterface::bind()
{
	if (auto result = libusb_open(_device, &_handle); result != 0)
		throw std::runtime_error("Unable to open UsbDeviceInterface");

	if (libusb_kernel_driver_active(_handle, 0) == 0)
	{
		auto ret = libusb_detach_kernel_driver(_handle, 0);
		if (ret != 0 && ret != LIBUSB_ERROR_NOT_FOUND)
		{
			// TODO: log that we failed to detach kernel driver
			throw std::runtime_error("Unable to detach kernel driver UsbDeviceInterface");
		}

		_reattach_kernel_driver = true;
	}

	if (libusb_claim_interface(_handle, 0) != 0)
	{
		// TODO: log that we failed to claim interface
		throw std::runtime_error("Unable to claim UsbDeviceInterface");
	}

	for (std::uint8_t conf_idx = 0; conf_idx < _device_desc.bNumConfigurations; ++conf_idx)
	{
		libusb_config_descriptor* config_desc = nullptr;
		if (auto result = libusb_get_config_descriptor(_device, conf_idx, &config_desc); result != 0)
			continue;

		for (std::uint8_t if_idx = 0; if_idx < config_desc->bNumInterfaces; ++if_idx)
		{
			for (std::uint8_t if_desc_idx = 0; if_desc_idx < config_desc->interface[if_idx].num_altsetting; ++if_desc_idx)
			{
				auto if_desc = config_desc->interface[if_idx].altsetting[if_desc_idx];
				for (std::uint8_t ep_idx = 0; ep_idx < if_desc.bNumEndpoints; ++ep_idx)
				{
					const auto& endpoint = if_desc.endpoint[ep_idx];
					_endpoint_mtu.emplace(ep_idx, endpoint.wMaxPacketSize);
				}
			}
		}
	}

	_release_interface = true;
}

std::uint32_t UsbDeviceInterface::get_vendor_id()
{
	return _device_desc.idVendor;
}

std::uint32_t UsbDeviceInterface::get_product_id()
{
	return _device_desc.idProduct;
}

void UsbDeviceInterface::control(std::uint32_t request_type, std::uint32_t request, std::uint32_t value)
{
	auto ret = libusb_control_transfer(
		_handle,
		request_type,
		request,
		value,
		0,
		nullptr,
		0,
		5000
	);

	// We seems to need to allow ERROR_PIPE as OK because
	// some control requests end with this request but
	// everything seems to be fine.
	if (ret != 0 && ret != LIBUSB_ERROR_PIPE)
		throw std::runtime_error("Unable to send control command to UsbDeviceInterface");
}

void UsbDeviceInterface::send(std::uint8_t endpoint, const Buffer& data)
{
	int sent = 0;
	int bytes_to_send = static_cast<int>(std::min(data.get_size(), get_endpoint_mtu(endpoint)));

	auto ret = libusb_bulk_transfer(
		_handle,
		LIBUSB_ENDPOINT_OUT | endpoint,
		const_cast<std::uint8_t*>(data.get_raw_data()),
		bytes_to_send,
		&sent,
		5000
	);

	if (ret != 0)
		throw std::runtime_error("Unable to send data to UsbDeviceInterface");
}

Buffer UsbDeviceInterface::recv(std::uint8_t endpoint)
{
	Buffer result(get_endpoint_mtu(endpoint));
	int read = 0;

	auto ret = libusb_bulk_transfer(
		_handle,
		LIBUSB_ENDPOINT_IN | endpoint,
		result.get_raw_data(),
		result.get_size(),
		&read,
		5000
	);

	if (ret != 0)
		throw std::runtime_error("Unable to receive data from UsbDeviceInterface");

	result.resize(read);
	return result;
}

std::size_t UsbDeviceInterface::get_endpoint_mtu(std::uint8_t endpoint)
{
	if (auto itr = _endpoint_mtu.find(endpoint); itr != _endpoint_mtu.end())
		return itr->second;
	else
		throw std::runtime_error("Unable to get endpoint MTU in UsbDeviceInterface");
}

} // namespace ccool
