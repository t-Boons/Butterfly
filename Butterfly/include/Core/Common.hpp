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
#include <numeric>

#include "glm/glm.hpp"
#include "glm/gtx/quaternion.hpp"
#include "entt/entt.hpp"

#include "Core/Log.hpp"
#include "Utils/Utils.hpp"
#include "Utils/Profiler.hpp"

// Supress warning macros
#define BF_WARNING_PUSH(x) __pragma(warning(push, x))
#define BF_WARNING_POP     __pragma(warning(pop))

#define BF_BIND_FUNC_PARAM(name) std::bind(name, this, std::placeholders::_1)
#define BF_BIND_FUNC(name) std::bind(name, this)

#define FREE(p)         \
    {if (p) {           \
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

    inline uint32_t Align256(uint32_t size)
    {
        return (size + 255) & ~255u;
    }

    template <typename T, typename... Args>
    inline RefPtr<T> StaticCastRef(Args&&... args)
    {
        return std::static_pointer_cast<T>(std::forward<Args>(args)...);
    }

    template <typename T, typename... Args>
    inline RefPtr<T> DynamicCastRef(Args&&... args)
    {
        return std::static_pointer_cast<T>(std::forward<Args>(args)...);
    }
}