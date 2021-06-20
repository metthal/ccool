#include <interfaces/interface.hpp>
#include <interfaces/debug/debug_interface.hpp>
#include <interfaces/usb/usb_interface.hpp>

namespace ccool {

std::unique_ptr<Interface> Interface::create(const std::string& name)
{
	if (name == "usb")
		return std::make_unique<UsbInterface>();
	else if (name == "debug")
		return std::make_unique<DebugInterface>();

	return nullptr;
}

} // namespace ccool
