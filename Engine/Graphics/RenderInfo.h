#pragma once

#include "SpriteId.h"

// 객체가 Renderer에게 전달하는 자료형
struct RenderInfo
{
	SpriteId spriteId = SpriteId::None;

	float x = 0.0f;
	float y = 0.0f;

	int frame = 0;
	int frameCount = 1;

	int frameWidthPx = 1;
	int frameHeightPx = 1;

	float renderHalfHeight = 0.18f;

	float offsetX = 0.0f;
	float offsetY = 0.0f;

	bool flipX = false;
	bool visible = true;
};