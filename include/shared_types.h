#pragma once
#include <atomic>

#ifdef NATIVE_BUILD
template <typename T>
using Shared = T;
#else
template <typename T>
using Shared = std::atomic<T>;
#endif
