#pragma once

#include <string>
#include <string_view>
#include <mutex>
#include <format>
#include <iostream>
#include <utility>
#include <Windows.h>

namespace tme::log {

enum class LogLevel {
	Info,
	Warning,
	Error
};

class Logger {
public:
	template<typename... Args>
	static void Log(
		LogLevel level,
		std::string_view category,
		const char* file,
		int line,
		std::format_string<Args...> fmt,
		Args&&... args)
	{
		std::string message = std::format(fmt, std::forward<Args>(args)...);
		Output(level, category, file, line, message);
	}

	static void SetMinLevel(LogLevel level)
	{
		std::scoped_lock lock(GetMutex());
		GetMinLevel() = level;
	}

private:
	static void Output(
		LogLevel level,
		std::string_view category,
		const char* file,
		int line,
		const std::string& message)
	{
		std::scoped_lock lock(GetMutex());

		if (level < GetMinLevel()) {
			return;
		}

		const char* levelStr = "[INFO]";
		WORD color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;

		switch (level)
		{
			case LogLevel::Info:
				levelStr = "[INFO]";
				color = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
				break;

			case LogLevel::Warning:
				levelStr = "[WARN]";
				color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
				break;

			case LogLevel::Error:
				levelStr = "[ERROR]";
				color = FOREGROUND_RED | FOREGROUND_INTENSITY;
				break;
		}

		std::string finalMsg = std::format(
			"{} [{}] [{}:{}] {}\n",
			levelStr,
			category,
			file,
			line,
			message
		);

		OutputDebugStringA(finalMsg.c_str());

		HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
		if (hConsole != INVALID_HANDLE_VALUE) {
			SetConsoleTextAttribute(hConsole, color);
		}

		std::cout << finalMsg;

		if (hConsole != INVALID_HANDLE_VALUE) {
			SetConsoleTextAttribute(
				hConsole,
				FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE
			);
		}
	}

	static std::mutex& GetMutex()
	{
		static std::mutex mutex;
		return mutex;
	}

	static LogLevel& GetMinLevel()
	{
		static LogLevel minLevel = LogLevel::Info;
		return minLevel;
	}
};

}