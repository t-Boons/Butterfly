#pragma once

#ifdef REL_WITH_PROFILING
#define ENABLE_PROFILING
#endif

#ifdef ENABLE_PROFILING

#define OPTICK_ENABLE_TRACING 1
#define USE_OPTICK 1
#define OPTICK_ENABLE_GPU_D3D12 1
#include "optick/include/optick.config.h"
#include "optick/include/optick.h"

#define BF_PROFILE_FRAME(...) OPTICK_FRAME(__VA_ARGS__)
#define BF_PROFILE_EVENT(...) OPTICK_EVENT(__VA_ARGS__)
#define BF_PROFILE_EVENT_DYNAMIC(str) OPTICK_EVENT_DYNAMIC(str)

#else

#define BF_PROFILE_FRAME(...) {}
#define BF_PROFILE_EVENT(...) {}
#define BF_PROFILE_EVENT_DYNAMIC(str) {}

#endif


