#pragma once

#include <Windows.h>
#include <string>
#include <stdexcept>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

namespace tme::sys::core {

class AppWindow {

public:
	AppWindow() = default;
	~AppWindow() = default;

	void Create(int width, int height, const std::wstring& title);
	bool ProcessMessage();
	void ShutDown();

private:
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

public:
	struct WindowDesc {
		std::wstring windowTitle = L"";
		uint32_t windowWidth = 1280;
		uint32_t windowHeight = 720;
		HINSTANCE hInstance = nullptr;
	};

#pragma region Accessor
	const HWND GetHWND() const { return hwnd_; }
	const WindowDesc& GetWindowDesc() const { return windowDesc_; }
#pragma endregion

private:
	HWND hwnd_ = nullptr;
	WindowDesc windowDesc_{};
	bool shouldClose_ = false;
	double aspectRatio_ = 16.0 / 9.0;
	int minClientWidth_ = 200;
	int minClientHeight_ = 200;
};

}