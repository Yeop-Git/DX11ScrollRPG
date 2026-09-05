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
	CreatePlayer();
	CreateMonsters();
	CreateItems();
}

void GameWorld::Update(float deltaTime)
{
	if (player_->IsDead())
	{
		if (GetAsyncKeyState('R') & 0x8000) Reset();
	}
	UpdateEntities(deltaTime);
	UpdatePhysics(deltaTime);

	// Collision 처리
	ResolveGroundCollisions();
	UpdateCombat();

	// Monster Pool 관리
	CollectDeadMonsters();
	UpdateMonsterLock();
	UpdateMonsterRespawn(deltaTime);

	UpdateItemPickup();
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

void GameWorld::CreatePlayer()
{
	// Create Player
	auto player = std::make_unique<Player>();
	player_ = player.get();
	entities_.push_back(player.get());
	gameObjects_.push_back(std::move(player));
}

void GameWorld::CreateMonsters()
{
	for (int i = 0; i < std::size(kMonsterUnlockKills); i++)
	{
		auto monster = std::make_unique<Monster>();
		monster->SetTarget(player_);

		Monster* monsterPtr = monster.get();

		monsters_.push_back(monsterPtr);
		entities_.push_back(monsterPtr);
		gameObjects_.push_back(std::move(monster));

		// 첫번째 몬스터만 Active하여 스폰
		monsterPtr->SetActive(i==0);
	}
}

void GameWorld::CreateItems()
{
	// Coin
	for (int i = 0; i < kCoinPoolSize; i++)
	{
		auto item = std::make_unique<WorldItem>(ItemType::Coin);

		WorldItem* ptr = item.get();

		ptr->SetActive(false);

		coinPool_.push(ptr);
		items_.push_back(ptr);
		entities_.push_back(ptr);
		gameObjects_.push_back(std::move(item));
	}

	// Potion
	for (int i = 0; i < kPotionPoolSize; i++)
	{
		auto item = std::make_unique<WorldItem>(ItemType::Potion);

		WorldItem* ptr = item.get();

		ptr->SetActive(false);

		potionPool_.push(ptr);
		items_.push_back(ptr);
		entities_.push_back(ptr);
		gameObjects_.push_back(std::move(item));
	}
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

void GameWorld::UpdatePhysics(float deltaTime)
{
	for (Entity* entity : entities_)
	{
		if (entity == nullptr) continue;
		if (!entity->IsActive()) continue;
		if (!entity->physics.enabled) continue;

		// isGrounded 초기화
		entity->physics.isGrounded = false;

		// 중력 적용
		if (entity->physics.useGravity)
		{
			entity->physics.velocity.y +=
				gravity_ * entity->physics.gravityScale * deltaTime;
		}

		entity->transform.position += entity->physics.velocity * deltaTime;
	}
}

void GameWorld::ResolveGroundCollisions()
{
	// 모든 월드의 Entity를 순회하며 Ground와의 충돌 계산
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

// Player와 Monster간의 충돌 계산, 둘다 Entity라 여기서 충돌 로직을 부여.ㄴ
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

void GameWorld::UpdateMonsterLock()
{
	while (unlockedMonsterCount_ < static_cast<int>(monsters_.size()))
	{
		const int nextIndex = unlockedMonsterCount_;

		if (killCount_ < kMonsterUnlockKills[nextIndex]) break;

		Monster* monster = monsters_[nextIndex];

		monster->SetActive(true);

		++unlockedMonsterCount_;
	}
}

void GameWorld::CollectDeadMonsters()
{
	// Dead이면서 애니메이션 끝난 몬스터 모아서 비활성화하고 respawnQueue에 집어넣기
	for (Monster* monster : monsters_)
	{
		if (!monster)
			continue;

		if (!monster->IsActive())
			continue;

		if (!monster->IsDeadAnimationFinished())
			continue;

		++killCount_;

		DropItem(monster->transform.position);

		monster->SetActive(false);
		respawnQueue_.push(monster);
	}
}

void GameWorld::UpdateMonsterRespawn(float deltaTime)
{
	if (respawnQueue_.empty())
	{
		respawnTimer_ = 0.0f;
		return;
	}

	respawnTimer_ += deltaTime;

	if (respawnTimer_ < kRespawnInterval) return;

	respawnTimer_ = 0.0f;

	Monster* monster = respawnQueue_.front();
	respawnQueue_.pop();

	monster->SetActive(true);
}

WorldItem* GameWorld::FindInactiveItem(ItemType type)
{
	for (WorldItem* item : items_)
	{
		if (!item->IsActive() && item->GetType() == type)
			return item;
	}
	return nullptr;
}

void GameWorld::DropItem(Vector2 position)
{
	const int roll = std::rand() % 100;
	if (roll < kPotionRate) DropPotion(position);
	else DropCoin(position);
}

void GameWorld::DropCoin(Vector2 position)
{
	if (coinPool_.empty()) return;

	WorldItem* coin = coinPool_.front();
	coinPool_.pop();

	coin->Spawn(position);
}

void GameWorld::DropPotion(Vector2 position)
{
	if (potionPool_.empty()) return;

	WorldItem* potion = potionPool_.front();
	potionPool_.pop();

	potion->Spawn(position);
}

void GameWorld::UpdateItemPickup()
{
	if (!player_)return;
	if(player_->IsDead())return;

	for (WorldItem* item : items_)
	{
		if (!item)continue;
		if(!item->IsActive())continue;

		if (!Intersects(player_->GetBodyBox(), item->GetBodyBox()))continue;

		if (item->GetType() == ItemType::Potion)
		{
			player_->Heal(1);
			potionPool_.push(item);
		}
		else if (item->GetType() == ItemType::Coin)
		{
			coinPool_.push(item);
		}

		item->SetActive(false);
	}
}

void GameWorld::Reset()
{
	if (player_ != nullptr)
	{
		player_->Reset();
	}

	killCount_ = 0;
	unlockedMonsterCount_ = 1;
	respawnTimer_ = 0.0f;

	// Monster Queue 초기화
	std::queue<Monster*> empty;
	std::swap(respawnQueue_, empty);

	// Monster들 초기화
	for (Monster* monster : monsters_)
	{
		if (!monster)continue;
		monster->SetActive(false);
	}

	if (!monsters_.empty())
	{
		monsters_[0]->SetActive(true);
	}

	// Item Queue 초기화
	std::queue<WorldItem*> emptyCoinPool;
	std::swap(coinPool_, emptyCoinPool);

	std::queue<WorldItem*> emptyPotionPool;
	std::swap(potionPool_, emptyPotionPool);

	// 모든 Item 회수
	for (WorldItem* item : items_)
	{
		if (!item)
			continue;

		item->SetActive(false);

		if (item->GetType() == ItemType::Coin)
		{
			coinPool_.push(item);
		}
		else if (item->GetType() == ItemType::Potion)
		{
			potionPool_.push(item);
		}
	}
}
