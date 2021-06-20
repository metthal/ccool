#pragma once

#include <memory>
#include <string>
#include <vector>

#include <interfaces/device_interface.hpp>

namespace ccool {

class Interface
{
public:
	Interface() = default;
	virtual ~Interface() = default;

	static std::unique_ptr<Interface> create(const std::string& name);

	virtual std::vector<std::unique_ptr<DeviceInterface>> get_device_interfaces() = 0;
};

} // namespace ccool
