#pragma once

#ifdef _DEBUG
#define BUTTERFLY_DEBUG
#endif

#ifdef NDEBUG
#define BUTTERFLY_RELEASE
#endif


#include <memory>
#include <queue>
#include <unordered_map>
#include <filesystem>
#include <fstream>
#include <vector>
#include <sstream>
#include <array>
#include <functional>
#include <type_traits>
#include <typeindex>
#include <string>
#include <any>
#include <exception>

#include "Utils/Log.hpp"
#include "Utils/Utils.hpp"
#include "Utils/Profiler.hpp"

// Supress warning macros
#define BF_WARNING_PUSH(x) __pragma(warning(push, x))
#define BF_WARNING_POP     __pragma(warning(pop))

#define BF_BIND_FUNC(name) std::bind(name, this, std::placeholders::_1)

#define FREE(p)         \
    {if (p) {            \
        delete p;       \
        p = nullptr;    \
    }}

namespace Butterfly
{
    template <typename T>
    using RefPtr = std::shared_ptr<T>;
    template <typename T>
    using ScopePtr = std::unique_ptr<T>;
    template <typename T>
    using WeakPtr = std::weak_ptr<T>;

    template <typename T, typename... Args>
    inline RefPtr<T> MakeRef(Args&&... args)
    {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }

    template <typename T, typename... Args>
    inline ScopePtr<T> MakeScope(Args&&... args)
    {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }

    template <typename T, typename... Args>
    inline WeakPtr<T> MakeWeak(Args&&... args)
    {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }
}