#include "Animator.h"

void Animator::Play(const AnimationClip& clip)
{
	currentClip_ = clip;

	currentFrame_ = 0;
	timer_ = 0.0f;

	finished_ = false;
}

void Animator::Update(float deltaTime)
{
	if (finished_) return;

	timer_ += deltaTime;

	if (timer_ < currentClip_.frameDuration) return;

	timer_ -= currentClip_.frameDuration;

	if (currentFrame_ < currentClip_.frameCount - 1)
	{
		++currentFrame_;
		return;
	}

	if (currentClip_.loop) currentFrame_ = 0;
	else finished_ = true;
}