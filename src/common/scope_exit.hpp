#pragma once

#include <functional>

namespace ccool {

class ScopeExit
{
public:
	template <typename F>
	ScopeExit(F&& callback) : _callback(std::forward<F>(callback)) {}

	~ScopeExit()
	{
		_callback();
	}

private:
	std::function<void()> _callback;
};

#define MAKE_UNIQUE_VAR_HELPER(prefix, suffix) prefix ## suffix
#define MAKE_UNIQUE_VAR(prefix, suffix)        MAKE_UNIQUE_VAR_HELPER(prefix, suffix)
#define on_scope_exit(body)                    auto MAKE_UNIQUE_VAR(scope_guard_, __LINE__) = ScopeExit( body )

} // namespace ccool
