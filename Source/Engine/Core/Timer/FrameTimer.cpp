#include "pch.h"
#include "FrameTimer.h"

namespace tme::sys::core {

FrameTimer::FrameTimer()
{
	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);
	frequency_ = freq.QuadPart;

	Reset();
}

void FrameTimer::Reset()
{
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);

	prevCounter_ = counter.QuadPart;

	deltaTime_ = 0.0f;
	totalTime_ = 0.0f;

	fps_ = 0.0f;
	fpsTimer_ = 0.0f;
	frameCount_ = 0;
}

void FrameTimer::Update()
{
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);

	int64_t current = counter.QuadPart;

	deltaTime_ = static_cast<float>(current - prevCounter_) /
		static_cast<float>(frequency_);

	prevCounter_ = current;

	// 安全クランプ
	if (deltaTime_ > 0.1f) {
		deltaTime_ = 0.1f;
	}

	totalTime_ += deltaTime_;

	// FPS計測
	fpsTimer_ += deltaTime_;
	frameCount_++;

	if (fpsTimer_ >= 1.0f) {
		fps_ = static_cast<float>(frameCount_) / fpsTimer_;
		frameCount_ = 0;
		fpsTimer_ = 0.0f;
	}
}

void FrameTimer::LimitFPS(float targetFPS) const
{
	if (targetFPS <= 0.0f) return;

	float targetTime = 1.0f / targetFPS;

	if (deltaTime_ < targetTime) {
		DWORD sleepTime = static_cast<DWORD>((targetTime - deltaTime_) * 1000.0f);
		Sleep(sleepTime);
	}
}

}