#pragma once

#include "../../Engine/Math/Vector2.h"

struct AnimationClip
{
	int frameCount = 1;
	float frameDuration = 0.1f;
	bool loop = true;

	// 렌더링 비율 자동 보정
	Vector2 frameSizePixels{ 1.0f, 1.0f };

	// 미세 조정
	Vector2 renderHalfSize{ 0.0f, 0.18f };
	Vector2 offset{ 0.0f, 0.0f };
};
