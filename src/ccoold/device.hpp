#pragma once

#include <string>

#include <interfaces/device_interface.hpp>

namespace ccool {

class BaseDevice
{
public:
	BaseDevice(const std::string& name, std::uint32_t fan_count, std::uint8_t data_endpoint) : _name(name), _fan_count(fan_count), _data_endpoint(data_endpoint) {}
	virtual ~BaseDevice() = default;

	const std::string& get_name() const { return _name; }
	std::uint32_t get_fan_count() const { return _fan_count; }

	virtual std::uint16_t read_pump_rpm() = 0;
	virtual std::vector<std::uint16_t> read_fans_rpm() = 0;
	virtual FixedPoint<16> read_temperature() = 0;
	virtual std::tuple<std::uint8_t, std::uint8_t, std::uint8_t> read_firmware_version() = 0;
	virtual void write_pump_mode(std::uint8_t mode) = 0;
	virtual void write_fans_pwm(std::uint8_t pwm) = 0;
	virtual void write_fans_rpm(std::uint16_t rpm) = 0;
	virtual void write_fans_curve(const std::vector<std::uint8_t>& temperatures, const std::vector<std::uint8_t>& pwms) = 0;

protected:
	std::string _name;
	std::uint32_t _fan_count;
	std::uint8_t _data_endpoint;
};

template <typename ProtocolT>
class Device : public BaseDevice
{
public:
	Device(std::unique_ptr<DeviceInterface>&& device_interface, const std::string& name, std::uint32_t fan_count, std::uint8_t data_endpoint)
		: BaseDevice(name, fan_count, data_endpoint), _device_interface(std::move(device_interface)), _protocol(_device_interface.get())
	{
		_device_interface->bind();
	}

	virtual ~Device() = default;

	virtual std::uint16_t read_pump_rpm() override
	{
		return _protocol.read_pump_rpm(_data_endpoint);
	}

	virtual std::vector<std::uint16_t> read_fans_rpm() override
	{
		std::vector<std::uint16_t> result(_fan_count, 0);
		for (std::uint8_t i = 0; i < _fan_count; ++i)
			std::tie(std::ignore, result[i]) = _protocol.read_fan_rpm(_data_endpoint, i);
		return result;
	}

	virtual FixedPoint<16> read_temperature() override
	{
		return _protocol.read_temperature(_data_endpoint);
	}

	virtual std::tuple<std::uint8_t, std::uint8_t, std::uint8_t> read_firmware_version() override
	{
		return _protocol.read_firmware_version(_data_endpoint);
	}

	virtual void write_pump_mode(std::uint8_t mode) override
	{
		_protocol.write_pump_mode(_data_endpoint, mode);
	}

	virtual void write_fans_pwm(std::uint8_t pwm) override
	{
		for (std::uint8_t i = 0; i < _fan_count; ++i)
			_protocol.write_fan_pwm(_data_endpoint, i, pwm);
	}

	virtual void write_fans_rpm(std::uint16_t rpm) override
	{
		for (std::uint8_t i = 0; i < _fan_count; ++i)
			_protocol.write_fan_rpm(_data_endpoint, i, rpm);
	}

	virtual void write_fans_curve(const std::vector<std::uint8_t>& temperatures, const std::vector<std::uint8_t>& pwms)
	{
		for (std::uint8_t i = 0; i < _fan_count; ++i)
			_protocol.write_fan_curve(_data_endpoint, i, temperatures, pwms);
	}

	void write_custom_led_color_enabled(std::uint8_t endpoint, std::uint8_t enabled);
	void write_custom_led_color(std::uint8_t endpoint, std::uint8_t red, std::uint8_t green, std::uint8_t blue);
	void write_enabled_external_temperature(std::uint8_t endpoint, std::uint8_t enabled);
	void write_external_temperature(std::uint8_t endpoint, const FixedPoint<16>& temperature);

protected:
	std::unique_ptr<DeviceInterface> _device_interface;
	ProtocolT _protocol;
};

} // namespace ccool
