#include <Windows.h>
#include "GameWorld.h"

#include "../Player.h"
#include "../Monster.h"
#include "../Collision/AABB.h"

void GameWorld::Initialize()
{
	CreateEntities();
}

void GameWorld::CreateEntities()
{
	entities_.clear();

	auto player = std::make_unique<Player>();
	player_ = player.get();

	entities_.push_back(std::move(player));

	auto monster = std::make_unique<Monster>();
	monster_ = monster.get();

	monster_->SetTarget(player_);

	entities_.push_back(std::move(monster));
}

void GameWorld::UpdateEntities(float deltaTime)
{
	for (auto& entity : entities_)
	{
		if (!entity->IsActive()) continue;
		entity->Update(deltaTime);
	}
}

void GameWorld::Update(float deltaTime)
{
	if (player_->IsDead())
	{
		if (GetAsyncKeyState('R') & 0x8000) Reset();
	}
	UpdateEntities(deltaTime);
	UpdateCombat();
}

void GameWorld::UpdateCombat()
{
	if (player_ == nullptr)return;

	if (player_->IsDead())return;

	if (!player_->collider.enabled)return;

	for (auto& entity : entities_)
	{
		auto* monster =
			dynamic_cast<Monster*>(entity.get());

		if (monster == nullptr)continue;

		if (monster->IsDead())continue;

		if (!monster->collider.enabled)continue;

		const AABB playerBody = player_->collider.GetBounds(player_->transform);

		const AABB monsterBody = monster->collider.GetBounds(monster->transform);

		// Player -> Monster
		if (player_->CanRegisterAttackHit())
		{
			if (Intersects(player_->GetAttackHitBox(), monsterBody))
			{
				monster->TakeDamage(1, player_->transform.x);

				player_->RegisterAttackHit();
			}
		}

		// Monster -> Player
		if (Intersects(playerBody, monsterBody))
		{
			player_->TakeDamage(1, monster->transform.x);
		}
	}
}

void GameWorld::Reset()
{
	if (player_ != nullptr)
	{
		player_->Reset();
	}

	if (monster_ != nullptr)
	{
		monster_->Reset();
	}
}