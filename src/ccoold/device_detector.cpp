#include <spdlog/spdlog.h>

#include "device_detector.hpp"
#include "devices/all.hpp"
#include "logging.hpp"

namespace ccool {

std::unique_ptr<BaseDevice> DeviceDetector::detect_device(const std::string& interface_name)
{
	spdlog::info("Detecting devices using interface '{}'...", interface_name);

	_interface = Interface::create(interface_name);
	if (!_interface)
		return nullptr;

	std::unique_ptr<BaseDevice> result;
	auto device_ifs = _interface->get_device_interfaces();
	for (auto&& device_if : device_ifs)
	{
		auto vendor_id = device_if->get_vendor_id();
		auto product_id = device_if->get_product_id();

		LOG->debug("  - {:#06x}:{:#06x}", vendor_id, product_id);
		result = check_known_devices(vendor_id, product_id, std::move(device_if));
		if (result)
		{
			LOG->debug("     - known device '{}'", result->get_name());
			break;
		}
	}

	return result;
}

} // namespace ccool
