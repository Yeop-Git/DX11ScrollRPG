#include "Ground.h"

Ground::Ground(Vector2 position, Vector2 halfSize)
	: GameObject(SpriteId::Ground, position, halfSize)
{
	collider.enabled = true;
}
