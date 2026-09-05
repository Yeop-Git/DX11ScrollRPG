#include "WorldItem.h"

WorldItem::WorldItem(ItemType type)
{
	type_ = type;
}

void WorldItem::Update(float deltaTime){}

void WorldItem::Spawn(Vector2 position)
{
	transform.position = position + spawnOffset;
	SetActive(true);
}

RenderInfo WorldItem::GetRenderInfo() const
{
	RenderInfo info;

	info.position = transform.position;
	info.renderHalfSize = { 0.0f,0.1f };

	// 두 원본 이미지 모두 1254×1254
	info.frameSizePixels = { 1254.0f, 1254.0f };

	switch (type_)
	{
	case ItemType::Coin:
		info.spriteId = SpriteId::Coin;
		break;
	case ItemType::Potion:
		info.spriteId = SpriteId::Potion;
		break;
	default:
		info.spriteId = SpriteId::None;
		break;
	}

	return info;
}