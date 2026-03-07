#pragma once

#include <cstdint>

namespace tme::sys::core {

class FrameTimer {

public:
	FrameTimer();
	~FrameTimer() = default;

	void Reset();
	void Update();

	void LimitFPS(float targetFPS) const;

#pragma region Accessor
	float GetDeltaTime() const { return deltaTime_; }
	float GetTotalTime() const { return totalTime_; }
	float GetFPS() const { return fps_; }
#pragma endregion

private:
	int64_t frequency_ = 0;
	int64_t prevCounter_ = 0;

	float deltaTime_ = 0.0f;
	float totalTime_ = 0.0f;

	// FPS計測
	float fps_ = 0.0f;
	float fpsTimer_ = 0.0f;
	int frameCount_ = 0;
};

}