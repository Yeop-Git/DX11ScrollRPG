#pragma once

struct AnimationClip
{
	int frameCount = 1;
	float frameDuration = 0.1f;
	bool loop = true;

	// 렌더링 비율 자동 보정
	int frameWidthPx = 1;
	int frameHeightPx = 1;

	// 미세 조정
	float renderHalfHeight = 0.18f;
	float offsetX = 0.0f;
	float offsetY = 0.0f;
};