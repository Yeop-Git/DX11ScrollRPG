#include "Animator.h"
#include <cmath>

void Animator::Play(const AnimationClip& clip)
{
	currentClip_ = clip;

	currentFrame_ = 0;
	timer_ = 0.0f;

	finished_ = false;
}

void Animator::Update(float deltaTime)
{
	if (finished_ || deltaTime <= 0.0f || !std::isfinite(deltaTime) || currentClip_.frameDuration <= 0.0f) return;
	const double elapsed = static_cast<double>(timer_) + deltaTime;
	const double steps = std::floor(elapsed / currentClip_.frameDuration);
	timer_ = static_cast<float>(std::fmod(elapsed, currentClip_.frameDuration));
	if (currentClip_.loop)
		currentFrame_ = static_cast<int>(std::fmod(currentFrame_ + steps, currentClip_.frameCount));
	else if (steps >= currentClip_.frameCount - currentFrame_)
	{
		currentFrame_ = currentClip_.frameCount - 1; finished_ = true;
	}
	else currentFrame_ += static_cast<int>(steps);
}
