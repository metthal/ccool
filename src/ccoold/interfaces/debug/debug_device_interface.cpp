#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include <interfaces/debug/debug_device_interface.hpp>

namespace ccool {

DebugDeviceInterface::DebugDeviceInterface() : _http_client(::getenv("CCOOLD_DEBUG_DEVICE_INTERFACE_SOCKET"), "debug_device_interface")
{
}

DebugDeviceInterface::~DebugDeviceInterface()
{
}

void DebugDeviceInterface::bind()
{
	auto response = _http_client.send_request("GET", "/bind");
}

std::uint32_t DebugDeviceInterface::get_vendor_id()
{
	auto response = _http_client.send_request("GET", "/vendor_id");
	return response.get_json()["vendor_id"].get<std::uint32_t>();
}

std::uint32_t DebugDeviceInterface::get_product_id()
{
	auto response = _http_client.send_request("GET", "/product_id");
	return response.get_json()["product_id"].get<std::uint32_t>();
}

void DebugDeviceInterface::control(std::uint32_t request_type, std::uint32_t request, std::uint32_t value)
{
	auto response = _http_client.send_request("POST", "/control", nlohmann::json{
		{"request_type", request_type},
		{"request", request},
		{"value", value}
	});
}

void DebugDeviceInterface::send(std::uint8_t endpoint, const Buffer& data)
{
	_http_client.send_request("POST", fmt::format("/send/{}", endpoint), nlohmann::json{
		{"data", data.get_hex_string()}
	});
}

Buffer DebugDeviceInterface::recv(std::uint8_t endpoint)
{
	auto response = _http_client.send_request("GET", fmt::format("/recv/{}", endpoint));
	return response.get_json()["data"].get<std::string>();
}

} // namespace ccool
