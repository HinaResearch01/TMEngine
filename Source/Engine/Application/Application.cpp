#include "pch.h"
#include "Application.h"

namespace tme {

int Application::Run()
{
	Init();
	Loop();
	Shutdown();
	return 0;
}

void Application::Init()
{
	int width = 1280, height = 720;	  // ウィンドウサイズ
	std::wstring title = L"TMEngine"; // タイトル

	// Window生成
	window_.Create(width, height, title);
	// FrmaeTimerリセット
	frame_timer_.Reset();

	// Graphic Engine初期化
	graphic_engine_.Init(window_.GetHWND(), width, height);
}

void Application::Loop()
{
	while (window_.ProcessMessage()) {
		frame_timer_.Update();

		// 描画準備
		graphic_engine_.BeginFrame();

		// 描画
		graphic_engine_.EndFrame();

		frame_timer_.LimitFPS(60.0f);
	}
}

void Application::Shutdown()
{
	graphic_engine_.Shutdown();
	window_.ShutDown();
}

}