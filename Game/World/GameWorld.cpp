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
	if (player_ == nullptr) return;
	if (monster_ == nullptr)return;

	if (player_->IsDead()) return;
	if (monster_->IsDead()) return;

	// 1. Player → Monster
	if (player_->IsAttackFrameActive() &&
		player_->CanRegisterAttackHit())
	{
		if (Intersects(
			player_->GetAttackHitBox(),
			monster_->GetBodyBox()))
		{
			monster_->TakeDamage(1, player_->GetX());
			player_->RegisterAttackHit();
		}
	}

	if (Intersects(
		player_->GetBodyBox(),
		monster_->GetBodyBox()))
	{
		player_->TakeDamage(1, monster_->GetX());
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