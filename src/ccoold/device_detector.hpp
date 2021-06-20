#pragma once

#include <memory>
#include <unordered_map>

#include "device.hpp"
#include "interfaces/device_interface.hpp"
#include "interfaces/interface.hpp"

namespace ccool {

struct DeviceId
{
	std::uint32_t vendor_id;
	std::uint32_t product_id;

	bool operator==(const DeviceId& rhs) const { return vendor_id == rhs.vendor_id && product_id == rhs.vendor_id; }
	bool operator!=(const DeviceId& rhs) const { return !(*this == rhs); }
};

class DeviceDetector
{
public:
	std::unique_ptr<BaseDevice> detect_device(const std::string& interface_name);

private:
	std::unique_ptr<Interface> _interface;
};

} // namespace ccool
