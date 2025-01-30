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

#ifdef BUTTERFLY_DEBUG
	#define Throw(...) Log::Critical(__VA_ARGS__);
	#define CheckMsg(...)    Log::Assert(__VA_ARGS__);
	#define Check(condition) assert(condition);
#else
	#define Throw(...) {}
	#define CheckMsg(...) {}
	#define Check(condition) {}
#endif

namespace Butterfly
{
	template <typename... Args>
	void InternalLog(const char* format, const char* color, Args... args)
	{
		time_t t = time(0);
		tm now;
		localtime_s(&now, &t);

		printf(color);
		printf("[Butterfly %i-%i-%i:%i:%i:%i] ", (int)now.tm_year + 1900, now.tm_mon + 1, now.tm_mday, now.tm_hour, now.tm_min, now.tm_sec);
		printf(format, args...);
		printf(RESET);
		printf("\n");
	}

	class Log
	{
	public:
		template <typename... Args>
		static void Info(const char* format, Args... args)
		{
			InternalLog(format, GREEN, args...);
		}

		template <typename... Args>
		static void Warn(const char* format, Args... args)
		{
			InternalLog(format, YELLOW, args...);
		}

		template <typename... Args>
		static void Error(const char* format, Args... args)
		{
			InternalLog(format, RED, args...);
		}

		template <typename... Args>
		static void Critical(const char* format, Args... args)
		{
			InternalLog(format, RED, args...);
			assert(false);
		}

		template <typename... Args>
		static void Assert(bool condition, const char* format, Args... args)
		{
			if (!condition)
			{
				Critical(format, args...);
			}
		}
	};
}