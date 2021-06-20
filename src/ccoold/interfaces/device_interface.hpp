#pragma once

#include <cstdint>
#include <memory>

#include "buffer.hpp"

namespace ccool {

class DeviceInterface
{
public:
	virtual ~DeviceInterface() = default;

	virtual void bind() = 0;

	virtual std::uint32_t get_vendor_id() = 0;
	virtual std::uint32_t get_product_id() = 0;

	virtual void control(std::uint32_t request_type, std::uint32_t request, std::uint32_t value) = 0;
	virtual void send(std::uint8_t endpoint, const Buffer& data) = 0;
	virtual Buffer recv(std::uint8_t endpoint) = 0;
};

} // namespace ccool
