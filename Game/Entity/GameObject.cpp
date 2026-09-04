#include "GameObject.h"

GameObject::GameObject(SpriteId spriteId, Vector2 position, Vector2 halfSize)
	: spriteId_(spriteId), renderSize_(halfSize)
{
	transform.position = position;
	collider.halfSize = halfSize;
	collider.enabled = false;
}

RenderInfo GameObject::GetRenderInfo() const
{
	RenderInfo info;
	info.spriteId = spriteId_;
	info.position = transform.position;
	info.renderHalfSize = renderSize_;
	return info;
}
