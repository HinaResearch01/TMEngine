#pragma once

#include <string>
#include <Windows.h>

namespace tme::util::str {

// UTF16(wstring) → UTF8(string)
inline std::string ToString(const std::wstring& wstr)
{
	if (wstr.empty())
		return {};

	int sizeNeeded = WideCharToMultiByte(
		CP_UTF8,
		0,
		wstr.c_str(),
		static_cast<int>(wstr.size()),
		nullptr,
		0,
		nullptr,
		nullptr);

	std::string result(sizeNeeded, 0);

	WideCharToMultiByte(
		CP_UTF8,
		0,
		wstr.c_str(),
		static_cast<int>(wstr.size()),
		result.data(),
		sizeNeeded,
		nullptr,
		nullptr);

	return result;
}

// UTF8(string) → UTF16(wstring)
inline std::wstring ToWString(const std::string& str)
{
	if (str.empty())
		return {};

	int sizeNeeded = MultiByteToWideChar(
		CP_UTF8,
		0,
		str.c_str(),
		static_cast<int>(str.size()),
		nullptr,
		0);

	std::wstring result(sizeNeeded, 0);

	MultiByteToWideChar(
		CP_UTF8,
		0,
		str.c_str(),
		static_cast<int>(str.size()),
		result.data(),
		sizeNeeded);

	return result;
}

}