#include <csignal>
#include <filesystem>
#include <thread>

#include <fmt/chrono.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/systemd_sink.h>
#include <ulocal/ulocal.hpp>

#include "ccool_daemon.hpp"
#include "daemonize.hpp"
#include "device_detector.hpp"
#include "logging.hpp"
#include "signals.hpp"

static volatile std::sig_atomic_t quit_requested = 0;

void handle_termination(int/* signum*/)
{
	quit_requested = 1;
}

namespace ccool {

CCoolDaemon::CCoolDaemon(const std::string& socket_path, bool daemonize) : _socket_path(socket_path), _daemonize(daemonize)
{
}

void CCoolDaemon::run(const std::string& interface)
{
	using namespace std::literals;

	std::shared_ptr<spdlog::logger> logger;
	if (_daemonize)
	{
		daemonize();
		logger = spdlog::systemd_logger_mt(LOGGER_NAME);
	}
	else
		logger = spdlog::stdout_color_mt(LOGGER_NAME);

	install_signal_handler(handle_termination, SIGINT, SIGTERM);
	LOG->set_level(spdlog::level::debug);
	LOG->error("Test error");

	DeviceDetector device_detector;
	auto device = device_detector.detect_device(interface);
	if (!device)
	{
		LOG->info("No device found. Exiting...");
		return;
	}

	std::filesystem::remove(_socket_path);
	ulocal::HttpServer ipc_server(_socket_path);

	ipc_server.endpoint({"GET"}, "/info", [&](const auto&) -> ulocal::HttpResponse {
		LOG->debug("IPC server request received - GET /info");
		return nlohmann::json{
			{"name", device->get_name()},
			{"fan_count", device->get_fan_count()}
		};
	});
	ipc_server.endpoint({"GET"}, "/pump", [&](const auto&) -> ulocal::HttpResponse {
		LOG->debug("IPC server request received - GET /pump");
		return nlohmann::json{
			{"rpm", device->read_pump_rpm()}
		};
	});
	ipc_server.endpoint({"GET"}, "/fans", [&](const auto&) -> ulocal::HttpResponse {
		LOG->debug("IPC server request received - GET /fans");
		return nlohmann::json{
			{"rpm", device->read_fans_rpm()}
		};
	});
	ipc_server.endpoint({"GET"}, "/temperature", [&](const auto&) -> ulocal::HttpResponse {
		LOG->debug("IPC server request received - GET /temperature");
		return nlohmann::json{
			{"temperature", device->read_temperature().floating()}
		};
	});
	ipc_server.endpoint({"GET"}, "/firmware", [&](const auto&) -> ulocal::HttpResponse {
		LOG->debug("IPC server request received - GET /firmware");
		auto version = device->read_firmware_version();
		return nlohmann::json{
			{"version", {
				{"major", std::get<0>(version)},
				{"minor", std::get<1>(version)},
				{"patch", std::get<2>(version)}
			}}
		};
	});
	ipc_server.endpoint({"POST"}, "/pump", [&](const auto& request) -> ulocal::HttpResponse {
		auto mode = request.get_json()["mode"].template get<std::uint8_t>();
		LOG->debug("IPC server request received - POST /pump mode={:d}", mode);
		device->write_pump_mode(mode);
		return nlohmann::json::object();
	});
	ipc_server.endpoint({"POST"}, "/fans", [&](const auto& request) -> ulocal::HttpResponse {
		auto request_json = request.get_json();
		if (request_json.find("rpm") != request_json.end())
		{
			auto rpm = request_json["rpm"].template get<std::uint16_t>();
			LOG->debug("IPC server request received - POST /fans rpm={:d}", rpm);
			device->write_fans_rpm(rpm);
		}
		else if (request_json.find("pwm") != request_json.end())
		{
			auto pwm = request_json["pwm"].template get<std::uint8_t>();
			LOG->debug("IPC server request received - POST /fans pwm={:d}", pwm);
			device->write_fans_pwm(pwm);
		}
		else if (request_json.find("curve") != request_json.end())
		{
			auto temperatures = std::vector<std::uint8_t>{};
			auto pwms = std::vector<std::uint8_t>{};
			for (const auto& point : request_json["curve"])
			{
				temperatures.push_back(point["temperature"].template get<std::uint8_t>());
				pwms.push_back(point["pwm"].template get<std::uint8_t>());
			}
			LOG->debug("IPC server request received - POST /fans temps=[{}] pwms=[{}]", fmt::join(temperatures.begin(), temperatures.end(), ", "), fmt::join(pwms.begin(), pwms.end(), ", "));
			device->write_fans_curve(temperatures, pwms);
		}
		else
		{
			LOG->warn("IPC server request received - POST /fans with unknown parameter");
			return {400, nlohmann::json{
				{"error", "Either 'rpm' or 'pwm' needs to be set."}
			}};
		}

		return nlohmann::json::object();
	});

	auto ipc_thread = std::thread([&]() {
		ipc_server.serve();
	});

	auto last_time = std::chrono::steady_clock::now();
	while (!quit_requested)
	{
		auto time_delta = std::chrono::steady_clock::now() - last_time;
		LOG->debug("Next iteration ({:%M:%S}.{:#03d} passed)", time_delta, time_delta.count() % 1000);

		last_time = std::chrono::steady_clock::now();
		std::this_thread::sleep_for(1s);
	}

	ipc_server.terminate();
	ipc_server.wait_until_done();
	ipc_thread.join();

	std::filesystem::remove(_socket_path);
}

} // namespace ccool
