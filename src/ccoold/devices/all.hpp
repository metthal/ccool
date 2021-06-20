#pragma once

#include <memory>

#include <device.hpp>
#include <interfaces/device_interface.hpp>

namespace ccool {

std::unique_ptr<BaseDevice> check_known_devices(std::uint32_t vendor_id, std::uint32_t product_id, std::unique_ptr<DeviceInterface>&& device_interface);

} // namespace ccool
