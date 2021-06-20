#pragma once

#include <signal.h>

namespace ccool {

using SignalHandler = void(*)(int);

namespace detail {

void op_signals(int op, sigset_t& signal_set)
{
	sigprocmask(op, &signal_set, nullptr);
}

template <typename... Args>
void op_signals(int op, sigset_t& signal_set, int signum, Args... signums)
{
	sigaddset(&signal_set, signum);
	op_signals(op, signal_set, signums...);
}

template <typename... Args>
void op_signals(int op, Args... signums)
{
	static_assert(sizeof...(Args) > 0, "op_signals needs to be called with at least one signal");

	sigset_t result;
	sigemptyset(&result);
	op_signals(op, result, signums...);
}

template <typename... Args>
void block_signals(Args... signums)
{
	op_signals(SIG_BLOCK, signums...);
}

template <typename... Args>
void unblock_signals(Args... signums)
{
	op_signals(SIG_UNBLOCK, signums...);
}

template <typename... Args>
void install_signal_handler(SignalHandler handler, int signum, Args... signums)
{
	struct sigaction action;
	action.sa_handler = handler;
	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;

	sigaction(signum, &action, nullptr);

	if constexpr (sizeof...(Args) > 0)
		install_signal_handler(handler, signums...);
}

} // namespace detail

template <typename... Args>
void install_signal_handler(SignalHandler handler, Args... signums)
{
	detail::block_signals(signums...);
	detail::install_signal_handler(handler, signums...);
	detail::unblock_signals(signums...);
}

} // namespace ccool
