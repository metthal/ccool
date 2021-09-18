#include <cstdlib>
#include <iostream>

#include <sys/types.h>
#include <unistd.h>

#include "daemonize.hpp"

namespace ccool {

void daemonize()
{
	auto pid = fork();
	if (pid < 0)
	{
		exit(EXIT_FAILURE);
		std::cerr << "Failed to perform first daemonization stage" << std::endl;
	}
	else if (pid > 0)
		exit(EXIT_SUCCESS);

	if (setsid() < 0)
	{
		std::cerr << "Failed to create session for daemon process" << std::endl;
		exit(EXIT_FAILURE);
	}

	pid = fork();
	if (pid < 0)
	{
		std::cerr << "Failed to perform second daemonization stage" << std::endl;
		exit(EXIT_FAILURE);
	}
	else if (pid > 0)
		exit(EXIT_SUCCESS);

	// TODO
	// umask();
	// chdir();

	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);
}

} // namespace ccool
