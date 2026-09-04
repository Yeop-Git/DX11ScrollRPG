#include <Windows.h>
#include "GameWorld.h"

#include "../Player.h"
#include "../Monster.h"
#include "../World/Ground.h"
#include "../Collision/AABB.h"

void GameWorld::Initialize()
{
	gameObjects_.clear();
	entities_.clear();
	grounds_.clear();

	CreateEnvironment();
	CreateGrounds();
	CreateEntities();
}

void GameWorld::CreateEnvironment()
{
	gameObjects_.push_back(std::make_unique<GameObject>(
		SpriteId::Background,
		Vector2{ 0.0f, 0.0f },
		Vector2{ 1.0f, 1.0f }));

	gameObjects_.push_back(std::make_unique<GameObject>(
		SpriteId::Tree,
		Vector2{ -0.65f, treeCenterY },
		treeHalfSize));

	gameObjects_.push_back(std::make_unique<GameObject>(
		SpriteId::Tree,
		Vector2{ 0.6f, treeCenterY },
		treeHalfSize));
}

void GameWorld::CreateEntities()
{
	// Create Player
	auto player = std::make_unique<Player>();
	player_ = player.get();
	entities_.push_back(player.get());
	gameObjects_.push_back(std::move(player));

	// Create Monster
	auto monster = std::make_unique<Monster>();
	monster->SetTarget(player_);
	entities_.push_back(monster.get());
	gameObjects_.push_back(std::move(monster));
}

void GameWorld::CreateGrounds()
{
	// groundCount만큼 ground를 생성
	for (int i = 0; i < groundCount; ++i)
	{
		Vector2 position = firstGroundPosition;
		position.x += static_cast<float>(i) * groundHalfSize.x * 1.5f;
		auto ground = std::make_unique<Ground>(position, groundHalfSize);
		grounds_.push_back(ground.get());
		gameObjects_.push_back(std::move(ground));
	}
}

void GameWorld::UpdateEntities(float deltaTime)
{
	for (Entity* entity : entities_)
	{
		if (entity == nullptr) continue;
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
	ResolveGroundCollisions();
	UpdateCombat();
}

void GameWorld::ResolveGroundCollisions()
{
	for (Entity* entity : entities_)
	{
		if (entity == nullptr) continue;
		if (!entity->IsActive()) continue;
		if (!entity->physics.enabled) continue;
		if (!entity->collider.enabled) continue;

		for (Ground* ground : grounds_)
		{
			if (ground == nullptr) continue;
			if (!ground->IsActive()) continue;
			if (!ground->collider.enabled) continue;

			if (!Intersects(entity->GetBodyBox(), ground->GetBodyBox())) continue;

			ResolveGroundCollision(*entity, *ground);
		}
	}
}

void GameWorld::ResolveGroundCollision(Entity& entity, const Ground& ground)
{
	if (entity.physics.velocity.y > 0.0f) return;

	const AABB groundBox = ground.GetBodyBox();

	entity.transform.position.y =
		groundBox.max.y
		+ entity.collider.halfSize.y
		- entity.collider.offset.y;

	entity.physics.velocity.y = 0.0f;
	entity.physics.isGrounded = true;
}

void GameWorld::UpdateCombat()
{
	if (player_ == nullptr)return;

	if (player_->IsDead())return;

	if (!player_->collider.enabled)return;

	for (auto& entity : entities_)
	{
		auto* monster = dynamic_cast<Monster*>(entity);

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
				monster->TakeDamage(1, player_->transform.position.x);

				player_->RegisterAttackHit();
			}
		}

		// Monster -> Player
		if (Intersects(playerBody, monsterBody))
		{
			player_->TakeDamage(1, monster->transform.position.x);
		}
	}
}

void GameWorld::Reset()
{
	if (player_ != nullptr)
	{
		player_->Reset();
	}

	for (Entity* entity : entities_)
	{
		auto* monster = dynamic_cast<Monster*>(entity);
		if (monster != nullptr) monster->Reset();
	}
}
