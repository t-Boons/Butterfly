#pragma once
#include "Core/Common.hpp"
#include <iostream>
#include <assert.h>
#include <stdio.h>

// ANSI escape codes for text colors
#define RESET "\033[0m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define GRAY "\033[90m"

#ifdef BUTTERFLY_DEBUG
#define BF_ASSERT(...)    Log::Assert("",  __VA_ARGS__)
#define BF_CORE_ASSERT(...)    Log::Assert("[CORE] ",  __VA_ARGS__)
#else
#define BF_ASSERT(...) {}
#define BF_CORE_ASSERT(...) {}
#endif

#define BF_CORE_LOG_TRACE(...)    Log::Trace("[CORE] ", __VA_ARGS__)
#define BF_CORE_LOG_INFO(...)     Log::Info("[CORE] ", __VA_ARGS__)
#define BF_CORE_LOG_WARN(...)     Log::Warn("[CORE] ", __VA_ARGS__)
#define BF_CORE_LOG_ERROR(...)    Log::Error("[CORE] ", __VA_ARGS__)
#define BF_CORE_LOG_CRITICAL(...) Log::Critical("[CORE] ", __VA_ARGS__)

#define BF_LOG_TRACE(...)    Log::Trace("", __VA_ARGS__)
#define BF_LOG_INFO(...)     Log::Info("", __VA_ARGS__)
#define BF_LOG_WARN(...)     Log::Warn("", __VA_ARGS__)
#define BF_LOG_ERROR(...)    Log::Error("", __VA_ARGS__)
#define BF_LOG_CRITICAL(...) Log::Critical("", __VA_ARGS__)

namespace Butterfly
{
	class Log
	{
	public:
		template <typename... Args>
		static void Trace(const char* prefix, const char* format, Args... args)
		{
			InternalLog(format, GRAY, prefix, args...);
		}

		template <typename... Args>
		static void Info(const char* prefix, const char*	format, Args... args)
		{
			InternalLog(format, GREEN, prefix, args...);
		}

		template <typename... Args>
		static void Warn(const char* prefix, const char* format, Args... args)
		{
			InternalLog(format, YELLOW, prefix, args...);
		}

		template <typename... Args>
		static void Error(const char* prefix, const char* format, Args... args)
		{
			InternalLog(format, RED, prefix, args...);
		}

		template <typename... Args>
		static void Critical(const char* prefix, const char* format, Args... args)
		{
			InternalLog(format, RED, prefix, args...);
			assert(false);
		}

		template <typename... Args>
		static void Assert(const char* prefix, bool condition, const char* format, Args... args)
		{
			if (!condition)
			{
				Critical(prefix, format, args...);
			 }
		}

	private:
		template <typename... Args>
		static void InternalLog(const char* format, const char* color, const char* prefix, Args... args)
		{
			time_t t = time(0);
			tm now;
			localtime_s(&now, &t);

			printf(color);
			printf("[Butterfly %i-%i-%i:%i:%i:%i] %s",
				(int)now.tm_year + 1900,
				now.tm_mon + 1,
				now.tm_mday,
				now.tm_hour,
				now.tm_min,
				now.tm_sec,
				prefix
			);

			printf(format, args...);
			printf(RESET);
			printf("\n");
		}
	};
}