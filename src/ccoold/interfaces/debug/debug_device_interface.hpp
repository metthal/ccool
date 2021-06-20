#pragma once

#include <ulocal/ulocal.hpp>

#include <interfaces/device_interface.hpp>

namespace ccool {

class DebugDeviceInterface : public DeviceInterface
{
public:
	DebugDeviceInterface();
	virtual ~DebugDeviceInterface();

	virtual void bind() override;

	virtual std::uint32_t get_vendor_id() override;
	virtual std::uint32_t get_product_id() override;

	virtual void control(std::uint32_t request_type, std::uint32_t request, std::uint32_t value) override;
	virtual void send(std::uint8_t endpoint, const Buffer& data) override;
	virtual Buffer recv(std::uint8_t endpoint) override;

private:
	ulocal::HttpClient _http_client;
	std::optional<ulocal::HttpResponse> _response;
};

} // namespace ccool
