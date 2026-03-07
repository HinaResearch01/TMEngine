#include "pch.h"
#include "ApplicationWindow.h"

namespace tme::sys::core {

void AppWindow::Create(int width, int height, const std::wstring& title)
{
	WindowDesc desc{};
	desc.windowTitle = title;
	desc.windowWidth = width;
	desc.windowHeight = height;
	desc.hInstance = GetModuleHandle(nullptr);
	LOG_INFO("Window", "Create window {}x{}", width, height);

	WNDCLASS wc{};
	wc.lpfnWndProc = WndProc;
	wc.hInstance = desc.hInstance;
	wc.lpszClassName = L"TME_WindowClass";
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

	wc.hbrBackground = nullptr;

	if (!RegisterClass(&wc)) {
		LOG_ERROR("Window", "RegisterClass failed");
		throw;
	}

	// 1. クライアントサイズからウィンドウ全体のサイズを逆算
	DWORD style = WS_OVERLAPPEDWINDOW;
	DWORD exStyle = 0;
	RECT clientRect = { 0, 0,
		static_cast<LONG>(desc.windowWidth),
		static_cast<LONG>(desc.windowHeight) };
	AdjustWindowRectEx(&clientRect, style, FALSE, exStyle);
	int windowWidth = clientRect.right - clientRect.left;
	int windowHeight = clientRect.bottom - clientRect.top;

	// 2. ウィンドウ生成
	hwnd_ = CreateWindowEx(
		exStyle,
		wc.lpszClassName,
		desc.windowTitle.c_str(),
		style,
		CW_USEDEFAULT, CW_USEDEFAULT,
		windowWidth, windowHeight,
		nullptr, nullptr, desc.hInstance, this
	);

	if (!hwnd_) {
		LOG_ERROR("Window", "CreateWindowEx failed");
		throw;
	}

	ShowWindow(hwnd_, SW_SHOW);
	UpdateWindow(hwnd_);

	// タイトルバーをダークモードに
	BOOL dark = TRUE;
	DwmSetWindowAttribute(
		hwnd_,
		DWMWA_USE_IMMERSIVE_DARK_MODE,
		&dark,
		sizeof(dark)
	);

	// タイトルバー色指定（Windows11）
	COLORREF color = RGB(30, 30, 30); // 好きな色
	DwmSetWindowAttribute(
		hwnd_,
		DWMWA_CAPTION_COLOR,
		&color,
		sizeof(color)
	);

	windowDesc_ = desc;
	aspectRatio_ = static_cast<float>(width) / height;

	LOG_INFO("Window", "Window created successfully");
}

bool AppWindow::ProcessMessage()
{
	MSG msg = {};
	while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
		if (msg.message == WM_QUIT) {
			shouldClose_ = true;
			return false;
		}

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return true;
}

void AppWindow::ShutDown()
{
	DestroyWindow(hwnd_);
	UnregisterClass(L"TME_WindowClass", windowDesc_.hInstance);
}

LRESULT AppWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_NCCREATE) {
		CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
		void* instancePtr = cs->lpCreateParams;
		SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(instancePtr));

		if (instancePtr) {
			AppWindow* win = reinterpret_cast<AppWindow*>(instancePtr);
			win->hwnd_ = hwnd;
		}
	}

	AppWindow* window = reinterpret_cast<AppWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
	if (window) {
		return window->HandleMessage(msg, wParam, lParam);
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT AppWindow::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
		case WM_ERASEBKGND:
			return 1;

		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;

		case WM_SIZE:
			return 0;

		case WM_SIZING: {
			RECT* prc = reinterpret_cast<RECT*>(lParam);
			RECT wndR, cliR;
			GetWindowRect(hwnd_, &wndR);
			GetClientRect(hwnd_, &cliR);

			int borderW = (wndR.right - wndR.left) - (cliR.right - cliR.left);
			int borderH = (wndR.bottom - wndR.top) - (cliR.bottom - cliR.top);

			int targetW = prc->right - prc->left - borderW;
			int targetH = prc->bottom - prc->top - borderH;

			// 比率維持ロジック
			if (wParam == WMSZ_LEFT || wParam == WMSZ_RIGHT ||
				wParam == WMSZ_TOPLEFT || wParam == WMSZ_TOPRIGHT ||
				wParam == WMSZ_BOTTOMLEFT || wParam == WMSZ_BOTTOMRIGHT) {
				targetH = static_cast<int>(targetW / aspectRatio_ + 0.5f);
			}
			else {
				targetW = static_cast<int>(targetH * aspectRatio_);
			}

			// サイズ適用
			int finalW = targetW + borderW;
			int finalH = targetH + borderH;

			if (wParam == WMSZ_TOP || wParam == WMSZ_TOPLEFT || wParam == WMSZ_TOPRIGHT)
				prc->top = prc->bottom - finalH;
			else
				prc->bottom = prc->top + finalH;

			if (wParam == WMSZ_LEFT || wParam == WMSZ_TOPLEFT || wParam == WMSZ_BOTTOMLEFT)
				prc->left = prc->right - finalW;
			else
				prc->right = prc->left + finalW;

			return TRUE;
		}
	}

	return DefWindowProc(hwnd_, msg, wParam, lParam);
}

}