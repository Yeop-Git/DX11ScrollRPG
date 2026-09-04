#pragma once

#include "../Collision/AABB.h"
#include "Transform.h"

struct Collider
{
	bool enabled = true;

	Vector2 offset{ 0.0f, 0.0f };
	Vector2 halfSize{ 0.05f, 0.05f };

	AABB GetBounds(const Transform& transform) const
	{
		const Vector2 center = transform.position + offset;

		return
		{
			center - halfSize,
			center + halfSize
		};
	}
};
