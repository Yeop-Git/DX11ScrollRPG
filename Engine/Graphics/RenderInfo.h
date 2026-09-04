#pragma once

#include "SpriteId.h"
#include "../Math/Vector2.h"

// 객체가 Renderer에게 전달하는 자료형
struct RenderInfo
{
	SpriteId spriteId = SpriteId::None;

	Vector2 position{ 0.0f, 0.0f };

	int frame = 0;
	int frameCount = 1;

	Vector2 frameSizePixels{ 1.0f, 1.0f };

	// x가 0이면 스프라이트 비율과 뷰포트 비율로 너비를 자동 계산한다.
	Vector2 renderHalfSize{ 0.0f, 0.18f };

	Vector2 offset{ 0.0f, 0.0f };

	bool flipX = false;
	bool visible = true;
};
