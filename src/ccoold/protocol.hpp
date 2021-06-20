#pragma once

#include <string>
#include <tuple>

#include "buffer.hpp"
#include "interfaces/device_interface.hpp"
#include "types.hpp"

namespace ccool {

template <Endian DataEndian, typename OpcodeTypeT>
class Protocol
{
public:
	static constexpr Endian endian = DataEndian;
	using OpcodeType = OpcodeTypeT;

	Protocol(DeviceInterface* device_interface, const std::string& name) : _device_interface(device_interface), _name(name) {}
	virtual ~Protocol() = default;

	const std::string& get_name() const { return _name; }

	Buffer send(std::uint8_t endpoint, const Buffer& data)
	{
		pre_request();
		_device_interface->send(endpoint, data);
		auto response = _device_interface->recv(endpoint);
		post_response();
		return response;
	}

	virtual void pre_request() {}
	virtual void post_response() {}

	virtual std::uint16_t read_pump_rpm(std::uint8_t endpoint) = 0;
	virtual std::tuple<std::uint32_t, std::uint16_t> read_fan_rpm(std::uint8_t endpoint, std::uint8_t fan_index) = 0;
	virtual FixedPoint<16> read_temperature(std::uint8_t endpoint) = 0;
	virtual std::tuple<std::uint8_t, std::uint8_t, std::uint8_t> read_firmware_version(std::uint8_t endpoint) = 0;
	virtual void write_pump_mode(std::uint8_t endpoint, std::uint8_t mode) = 0;
	virtual void write_fan_curve(std::uint8_t endpoint, std::uint8_t fan_index, const std::vector<std::uint8_t>& temperatures, const std::vector<std::uint8_t>& pwm) = 0;
	virtual void write_fan_pwm(std::uint8_t endpoint, std::uint8_t fan_index, std::uint8_t pwm) = 0;
	virtual void write_fan_rpm(std::uint8_t endpoint, std::uint8_t fan_index, std::uint16_t rpm) = 0;
	virtual void write_custom_led_color_enabled(std::uint8_t endpoint, std::uint8_t enabled) = 0;
	virtual void write_custom_led_color(std::uint8_t endpoint, std::uint8_t red, std::uint8_t green, std::uint8_t blue) = 0;
	virtual void write_enabled_external_temperature(std::uint8_t endpoint, std::uint8_t enabled) = 0;
	virtual void write_external_temperature(std::uint8_t endpoint, const FixedPoint<16>& temperature) = 0;

protected:
	DeviceInterface* _device_interface;

private:
	std::string _name;
};

} // namespace ccool
