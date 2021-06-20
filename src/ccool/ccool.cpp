#include <ranges>

#include <cxxopts.hpp>
#include <fmt/color.h>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <nlohmann/json.hpp>
#include <ulocal/ulocal.hpp>

#include <conversion.hpp>
#include <string.hpp>

void print_data(bool show_json, const nlohmann::json& json)
{
	if (show_json)
	{
		fmt::print("{}\n", json.dump(2));
	}
	else
	{
		fmt::print("Non JSON output is not supported yet.\n");
	}
}

ulocal::HttpResponse send_request(ulocal::HttpClient& client, const std::string& method, const std::string& resource, std::optional<nlohmann::json> data = std::nullopt)
{
	using namespace std::literals;

	for (std::uint32_t try_count = 0; try_count < 50; ++try_count)
	{
		try
		{
			if (data)
				return client.send_request(method, resource, data.value());
			else
				return client.send_request(method, resource);
		}
		catch (const ulocal::SocketError& error)
		{
			if (try_count == 49)
				throw;
			else
				std::this_thread::sleep_for(100ms);
		}
	}

	return {};
}

int main(int argc, char* argv[])
{
	cxxopts::Options options("ccool", "CCool CLI client");
	options.add_options()
		("h,help", "Show usage")
		("j,json", "Show raw output in form of JSON")
		("s,socket", "Use specified socket", cxxopts::value<std::string>()->default_value("/var/run/ccool/ccoold.sock"))
		("v,verbose", "Verbose logging messages")
		("version", "Show version information")
		;

	auto result = options.parse(argc, argv);
	auto json_output = result.count("json") > 0;

	if (result.count("help"))
	{
		fmt::print(options.help());
		return 0;
	}

	auto commands = result.unmatched();
	if (commands.empty())
	{
		fmt::print(options.help());
		return 1;
	}

	ulocal::HttpClient client(result["socket"].as<std::string>());
	ulocal::HttpResponse response;

	if (commands[0] == "info")
		response = send_request(client, "GET", "/info");
	else if (commands[0] == "pump")
	{
		if (commands.size() == 1)
			response = send_request(client, "GET", "/pump");
		else
			response = send_request(client, "POST", "/pump", nlohmann::json{
				{"mode", ccool::convert<std::uint8_t>(commands[1]).value()}
			});
	}
	else if (commands[0] == "fans")
	{
		if (commands.size() == 1)
			response = send_request(client, "GET", "/fans");
		else
		{
			if (commands[1] == "pwm")
				response = send_request(client, "POST", "/fans", nlohmann::json{
					{"pwm", ccool::convert<std::uint8_t>(commands[2]).value()}
				});
			else if (commands[1] == "rpm")
				response = send_request(client, "POST", "/fans", nlohmann::json{
					{"rpm", ccool::convert<std::uint16_t>(commands[2]).value()}
				});
			else if (commands[1] == "curve")
			{
				fmt::print("HERE\n");
				commands.erase(commands.begin(), commands.begin() + 2);
				auto curve_points = nlohmann::json::array();
				for (const auto& command : commands)
				{
					auto temp_pwm = ccool::split(command, '-');
					if (temp_pwm.size() != 2)
					{
						fmt::print(stderr, "Fan curve needs to specified in format <TEMP>-<PWM>\n");
						return 1;
					}

					fmt::print("{}\n", fmt::join(temp_pwm, "-"));

					curve_points.push_back(nlohmann::json{
						{"temperature", ccool::convert<std::uint8_t>(temp_pwm[0]).value()},
						{"pwm", ccool::convert<std::uint8_t>(temp_pwm[1]).value()}
					});
				}

				response = send_request(client, "POST", "/fans", nlohmann::json{
					{"curve", curve_points}
				});
			}
		}
	}
	else if (commands[0] == "temp")
		response = send_request(client, "GET", "/temperature");
	else if (commands[0] == "firmware")
		response = send_request(client, "GET", "/firmware");
	else
	{
		fmt::print(stderr, "Unknown command: {}", commands[0]);
		return 1;
	}

	if (response.get_status_code() != 200)
	{
		fmt::print(stderr, "Request failed!\n\n{}\n", response.get_content());
		return 2;
	}

	//fmt::print(stderr, "{}\n", response.dump());
	auto response_data = response.get_json();
	print_data(json_output, response_data);

	//fmt::print(
	//	fmt::emphasis::bold | fmt::fg(fmt::color::white),
	//	"Hello\n"
	//);
	return 0;
}
