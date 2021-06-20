#pragma once

#include "device_detector.hpp"

namespace ccool {

class CCoolDaemon
{
public:
	CCoolDaemon(const std::string& socket_path);

	void run(const std::string& interface);

private:
	std::string _socket_path;
};

} // namespace ccool
