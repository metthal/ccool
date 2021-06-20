#include <interfaces/debug/debug_interface.hpp>
#include <interfaces/debug/debug_device_interface.hpp>
#include <scope_exit.hpp>

namespace ccool {

DebugInterface::DebugInterface()
{
}

DebugInterface::~DebugInterface()
{
}

std::vector<std::unique_ptr<DeviceInterface>> DebugInterface::get_device_interfaces()
{
	std::vector<std::unique_ptr<DeviceInterface>> result;
	result.push_back(std::make_unique<DebugDeviceInterface>());
	return result;
}

} // namespace ccool
