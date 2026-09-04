#pragma once

#include "../Entity/GameObject.h"

class Ground : public GameObject
{
public:
	Ground(Vector2 position, Vector2 halfSize);
};
