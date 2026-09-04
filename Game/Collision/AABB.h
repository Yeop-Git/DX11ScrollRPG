#pragma once

#include "../../Engine/Math/Vector2.h"

// Axis-Aligned Bounding Box, 회전하지 않는 사각 콜라이더
struct AABB
{
	Vector2 min;
	Vector2 max;
};

inline bool Intersects(const AABB& a, const AABB& b)
{
	return
		a.min.x < b.max.x &&
		a.max.x > b.min.x &&
		a.min.y < b.max.y &&
		a.max.y > b.min.y;
}
