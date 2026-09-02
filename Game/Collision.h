#pragma once

// Axis-Aligned Bounding Box, 회전하지 않는 사각 콜라이더
struct AABB
{
	float left;
	float right;
	float bottom;
	float top;
};

inline bool Intersects(const AABB& a, const AABB& b)
{
	return
		a.left < b.right &&
		a.right > b.left &&
		a.bottom < b.top &&
		a.top > b.bottom;
}