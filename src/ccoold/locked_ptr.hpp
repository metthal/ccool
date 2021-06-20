#pragma once

#include <mutex>

namespace ccool {

template <typename T, typename Mutex>
class locked_ptr
{
public:
	template <typename TT>
	locked_ptr(TT&& value, Mutex& mutex)
		: _value(std::forward<TT>(value)), _mutex(mutex), _lock(_mutex) {}
	locked_ptr(const locked_ptr&) = delete;
	locked_ptr(locked_ptr&&) noexcept = default;

	locked_ptr& operator=(const locked_ptr&) = delete;
	locked_ptr& operator=(locked_ptr&&) noexcept = default;

	T* operator->() const { return _value; }

private:
	T* _value;
	Mutex& _mutex;
	std::unique_lock<Mutex> _lock;
};

template <typename T, typename Mutex>
locked_ptr(T*, Mutex&) -> locked_ptr<T, Mutex>;

} // namespace ccool
