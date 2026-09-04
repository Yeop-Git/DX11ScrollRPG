#pragma once

#include "../../Engine/Math/Vector2.h"

struct Physics
{
	bool enabled = true;
	bool useGravity = true;

	Vector2 velocity = { 0.0f, 0.0f };

	float gravityScale = 1.0f;

	bool isGrounded = false;
};