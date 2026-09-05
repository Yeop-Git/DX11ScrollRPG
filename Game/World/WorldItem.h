#pragma once

#include "../Entity/Entity.h"

enum class ItemType
{
	Coin,
	Potion
};

class WorldItem : public Entity
{
public:
	WorldItem(ItemType type);
	void Update(float deltaTime) override;

	RenderInfo GetRenderInfo() const override;

	ItemType GetType() const { return type_; }

	void Spawn(Vector2 position);
private:
	ItemType type_;
	static constexpr Vector2 spawnOffset{ 0.0f, 0.1f };
};