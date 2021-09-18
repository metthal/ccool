#include <cxxopts.hpp>
#include <fmt/core.h>
#include <fmt/format.h>

#include "ccool_daemon.hpp"

int main(int argc, char* argv[])
{
	cxxopts::Options options("ccoold", "CCool daemon");
	options.add_options()
		("h,help", "Show usage")
		("i,interface", "Interface to use", cxxopts::value<std::string>()->default_value("usb"))
		("n,no-daemon", "Do not run daemonized")
		("s,socket", "Use specified socket", cxxopts::value<std::string>()->default_value("/var/run/ccool/ccoold.sock"))
		("v,verbose", "Verbose logging messages")
		("version", "Show version information")
		;

	auto result = options.parse(argc, argv);

	if (result.count("help"))
	{
		fmt::print(options.help());
		return 0;
	}

	ccool::CCoolDaemon ccool_daemon(result["socket"].as<std::string>(), result["no-daemon"].count() == 0u);
	ccool_daemon.run(result["interface"].as<std::string>());
	return 0;
}
