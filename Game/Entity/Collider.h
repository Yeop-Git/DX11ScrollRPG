#pragma once

#include "../Collision/AABB.h"

struct Collider
{
	bool enabled = true;

	float offsetX = 0.0f;
	float offsetY = 0.0f;

	float halfWidth = 0.05f;
	float halfHeight = 0.05f;

	AABB GetBounds(const Transform& transform) const
	{
		const float centerX = transform.x + offsetX;
		const float centerY = transform.y + offsetY;

		return
		{
			centerX - halfWidth,
			centerX - halfWidth,
			centerY - halfHeight,
			centerY + halfHeight
		};
	}
};