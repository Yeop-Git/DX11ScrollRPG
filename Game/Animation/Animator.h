#pragma once

#include "AnimationClip.h"

class Animator
{
public:
	void Play(const AnimationClip& clip);

	void Update(float deltaTime);

	int GetCurrentFrame() const
	{
		return currentFrame_;
	}

	int GetFrameCount() const
	{
		return currentClip_.frameCount;
	}

	const AnimationClip& GetCurrentClip() const
	{
		return currentClip_;
	}

	bool IsFinished() const
	{
		return finished_;
	}

private :
	AnimationClip currentClip_{};

	int currentFrame_ = 0;
	float timer_ = 0.0f;

	bool finished_ = false;
};