#pragma once

#include <interfaces/device_interface.hpp>
#include <interfaces/interface.hpp>

namespace ccool {

class DebugInterface : public Interface
{
public:
	DebugInterface();
	virtual ~DebugInterface();

	virtual std::vector<std::unique_ptr<DeviceInterface>> get_device_interfaces() override;
};

} // namespace ccool
