#pragma once

#include "Engine/Core/Window/ApplicationWindow.h"
#include "Engine/Core/Timer/FrameTimer.h"
#include "Engine/Graphics/GraphicsEngine.h"

namespace tme {

class Application {

public:
	Application() = default;
	~Application() = default;
	int Run();

private:
	void Init();
	void Loop();
	void Shutdown();

private:
	tme::sys::core::AppWindow window_;
	tme::sys::core::FrameTimer frame_timer_;

	tme::sys::graphics::GraphicsEngine graphic_engine_;
};

}