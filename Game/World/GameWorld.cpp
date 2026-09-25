#include <Windows.h>
#include <algorithm>
#include <cmath>
#include <unordered_set>
#include "GameWorld.h"

#include "../Player.h"
#include "../Monster.h"
#include "../World/Ground.h"
#include "../Collision/AABB.h"

void GameWorld::Initialize()
{
	for (auto& layer : renderLayers_) layer.clear();
	gameObjects_.clear();
	entities_.clear();
	grounds_.clear();

	CreateEnvironment();
	CreateGrounds();
	CreatePlayer();
	CreateMonsters();
	CreateItems();
}

GameObject* GameWorld::AddGameObject(std::unique_ptr<GameObject> object, RenderLayer layer)
{
	GameObject* objectReference = object.get();
	gameObjects_.push_back(std::move(object));

	// 각 레이어에 고정 깊이 구간을 나눠 등록 순서가 안정적인 깊이 우선순위가 되게 한다.
	constexpr float kDepthStart[] = { 0.05f, 0.20f, 0.50f, 0.90f, 0.40f };
	constexpr float kDepthEnd[] = { 0.15f, 0.35f, 0.70f, 0.90f, 0.45f };
	constexpr std::size_t kLayerCapacity[] = { 1, 50003, 32, 1, 8 };

	auto& layerObjects = renderLayers_[static_cast<std::size_t>(layer)];
	const std::size_t layerIndex = layerObjects.size();
	const std::size_t capacity = kLayerCapacity[static_cast<std::size_t>(layer)];
	const float interpolation = capacity <= 1
		? 0.0f
		: static_cast<float>(layerIndex) / static_cast<float>(capacity - 1);
	const float depth = kDepthStart[static_cast<std::size_t>(layer)]
		+ (kDepthEnd[static_cast<std::size_t>(layer)]
			- kDepthStart[static_cast<std::size_t>(layer)]) * interpolation;

	layerObjects.push_back({ objectReference, depth });
	return objectReference;
}

void GameWorld::RemoveRenderObjects(const std::unordered_set<const GameObject*>& objects)
{
	for (auto& layer : renderLayers_)
	{
		std::erase_if(layer, [&objects](const RenderObject& renderObject)
		{
			return objects.contains(renderObject.object);
		});
	}
}

void GameWorld::Update(float deltaTime, Profiler& profiler)
{
	if (player_->IsDead())
	{
		if (stressMonsters_.empty() && (GetAsyncKeyState('R') & 0x8000)) Reset();
	}
	UpdateEntities(deltaTime, profiler);
	UpdatePhysics(deltaTime, profiler);

	// Collision 처리
	ResolveGroundCollisions(profiler);
	UpdateCombat(profiler);

	// Monster Pool 관리
	CollectDeadMonsters();
	UpdateMonsterLock();
	UpdateMonsterRespawn(deltaTime);

	UpdateItemPickup(profiler);
}

void GameWorld::CreateEnvironment()
{
	AddGameObject(std::make_unique<GameObject>(
		SpriteId::Background,
		Vector2{ 0.0f, 0.0f },
		Vector2{ 1.0f, 1.0f }), RenderLayer::Background);

	AddGameObject(std::make_unique<GameObject>(
		SpriteId::Tree,
		Vector2{ -0.65f, treeCenterY },
		treeHalfSize), RenderLayer::Environment);

	AddGameObject(std::make_unique<GameObject>(
		SpriteId::Tree,
		Vector2{ 0.6f, treeCenterY },
		treeHalfSize), RenderLayer::Environment);
}

void GameWorld::CreatePlayer()
{
	// Create Player
	auto player = std::make_unique<Player>();
	player_ = player.get();
	entities_.push_back(player.get());
	AddGameObject(std::move(player), RenderLayer::Player);
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
		AddGameObject(std::move(monster), RenderLayer::Monster);

		// 첫번째 몬스터만 Active하여 스폰
		monsterPtr->SetActive(i==0);
	}
}

void GameWorld::SetStressTestMonsterCount(std::size_t count)
{
	// UI 버튼에서 사용하는 네 단계만 허용해 의도하지 않은 대량 생성을 막는다.
	if (count != 0 && count != 1000 && count != 5000
		&& count != 10000 && count != 50000)
	{
		return;
	}

	ClearStressTestMonsters();
	Reset();

	if (count == 0)
	{
		return;
	}

	// 기본 게임 Monster는 잠시 비활성화해 요청된 테스트 수만 측정한다.
	for (Monster* monster : monsters_)
	{
		if (monster != nullptr)
		{
			monster->SetActive(false);
		}
	}

	CreateStressTestMonsters(count);
}

void GameWorld::CreateStressTestMonsters(std::size_t count)
{
	// 화면 비율을 고려한 격자로 고정 배치해 매번 같은 분포를 만든다.
	constexpr float kLeft = -0.92f;
	constexpr float kRight = 0.92f;
	constexpr float kTop = 0.82f;
	constexpr float kBottom = -0.82f;
	constexpr float kAspectRatio = 1280.0f / 720.0f;

	const std::size_t columns = static_cast<std::size_t>(
		std::ceil(std::sqrt(static_cast<double>(count) * kAspectRatio)));
	const std::size_t rows = (count + columns - 1) / columns;
	const float cellWidth = (kRight - kLeft) / static_cast<float>(columns);
	const float cellHeight = (kTop - kBottom) / static_cast<float>(rows);

	stressMonsters_.reserve(count);
	stressMonsterLookup_.reserve(count);
	entities_.reserve(entities_.size() + count);
	gameObjects_.reserve(gameObjects_.size() + count);

	for (std::size_t i = 0; i < count; ++i)
	{
		auto monster = std::make_unique<Monster>();
		Monster* monsterPtr = monster.get();

		// OnEnable이 기본 물리·Collider 값을 복원한 뒤 스트레스 설정을 덮어쓴다.
		monsterPtr->SetActive(false);
		monsterPtr->SetActive(true);
		monsterPtr->SetTarget(nullptr);
		monsterPtr->physics.enabled = true;
		monsterPtr->physics.useGravity = false;
		monsterPtr->physics.velocity = {};
		monsterPtr->collider.enabled = true;

		const std::size_t column = i % columns;
		const std::size_t row = i / columns;
		monsterPtr->transform.position = {
			kLeft + (static_cast<float>(column) + 0.5f) * cellWidth,
			kTop - (static_cast<float>(row) + 0.5f) * cellHeight
		};

		stressMonsters_.push_back(monsterPtr);
		stressMonsterLookup_.insert(monsterPtr);
		entities_.push_back(monsterPtr);
		AddGameObject(std::move(monster), RenderLayer::Monster);
	}
}

void GameWorld::ClearStressTestMonsters()
{
	if (stressMonsters_.empty())
	{
		return;
	}

	// 비소유 참조를 지우기 전에 대상 포인터 집합을 만들어 안전하게 찾아낸다.
	std::unordered_set<const GameObject*> stressObjects;
	stressObjects.reserve(stressMonsters_.size());
	for (Monster* monster : stressMonsters_)
	{
		stressObjects.insert(monster);
		monster->SetActive(false);
	}

	std::erase_if(entities_, [&stressObjects](Entity* entity)
	{
		return stressObjects.contains(entity);
	});
	RemoveRenderObjects(stressObjects);
	std::erase_if(gameObjects_, [&stressObjects](const std::unique_ptr<GameObject>& object)
	{
		return stressObjects.contains(object.get());
	});
	stressMonsters_.clear();
	stressMonsterLookup_.clear();
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
		AddGameObject(std::move(item), RenderLayer::TransparentItem);
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
		AddGameObject(std::move(item), RenderLayer::TransparentItem);
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
		AddGameObject(std::move(ground), RenderLayer::Environment);
	}
}

void GameWorld::UpdateEntities(float deltaTime, Profiler& profiler)
{
	// 이 구간은 Entity 게임 로직과 애니메이션 Update를 포함한다.
	ProfileScope scope(profiler, ProfileCategory::EntityUpdate);
	for (Entity* entity : entities_)
	{
		if (entity == nullptr) continue;
		if (!entity->IsActive()) continue;
		// 실제 Update 대상으로 선택된 활성 Entity 수를 프레임 카운터에 반영한다.
		profiler.Increment(ProfileCounter::ActiveEntities);
		entity->Update(deltaTime);
	}
}

void GameWorld::UpdatePhysics(float deltaTime, Profiler& profiler)
{
	// 중력 적용과 Transform 위치 갱신에 걸린 시간만 측정한다.
	ProfileScope scope(profiler, ProfileCategory::Physics);
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

void GameWorld::ResolveGroundCollisions(Profiler& profiler)
{
	// Ground 후보 순회와 AABB 판정을 포함한 총 시간을 측정한다.
	ProfileScope scope(profiler, ProfileCategory::GroundCollision);
	// 모든 월드의 Entity를 순회하며 Ground와의 충돌 계산
	for (Entity* entity : entities_)
	{
		if (entity == nullptr) continue;
		if (!entity->IsActive()) continue;
		if (!entity->physics.enabled) continue;
		if (!entity->collider.enabled) continue;

		const bool isStressMonster = stressMonsterLookup_.contains(entity);
		for (Ground* ground : grounds_)
		{
			if (ground == nullptr) continue;
			if (!ground->IsActive()) continue;
			if (!ground->collider.enabled) continue;

			// 실제 Intersects 호출 횟수를 세어 Broad Phase 전 Baseline으로 사용한다.
			profiler.Increment(ProfileCounter::CollisionChecks);
			if (!Intersects(entity->GetBodyBox(), ground->GetBodyBox())) continue;
			// AABB 비용은 측정하되 고정 격자 위치를 지면 반응으로 바꾸지 않는다.
			if (isStressMonster) continue;

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

// Player와 Monster간의 충돌 계산, 둘다 Entity라 여기서 충돌 로직을 부여
void GameWorld::UpdateCombat(Profiler& profiler)
{
	// Player와 Monster 사이의 공격 및 몸체 AABB 판정 시간을 측정한다.
	ProfileScope scope(profiler, ProfileCategory::CombatCollision);
	if (player_ == nullptr)return;

	if (player_->IsDead())return;

	if (!player_->collider.enabled)return;

	for (auto& monster : monsters_)
	{
		if (monster == nullptr)continue;

		if (!monster->IsActive()) continue;

		if (monster->IsDead())continue;

		if (!monster->collider.enabled)continue;

		const AABB playerBody = player_->collider.GetBounds(player_->transform);

		const AABB monsterBody = monster->collider.GetBounds(monster->transform);

		// Player -> Monster
		if (player_->CanRegisterAttackHit())
		{
			// 공격 HitBox가 실제 검사 대상이 된 경우에만 검사 수를 올린다.
			profiler.Increment(ProfileCounter::CollisionChecks);
			if (Intersects(player_->GetAttackHitBox(), monsterBody))
			{
				monster->TakeDamage(1, player_->transform.position.x);

				player_->RegisterAttackHit();
			}
		}

		// Monster -> Player
		// 활성 Monster의 몸체와 Player 몸체 간 실제 검사 횟수다.
		profiler.Increment(ProfileCounter::CollisionChecks);
		if (Intersects(playerBody, monsterBody))
		{
			player_->TakeDamage(1, monster->transform.position.x);
		}
	}

	// 스트레스 Monster도 매 프레임 Player 몸체와 검사하되 전투 효과는 발생시키지 않는다.
	const AABB playerBody = player_->collider.GetBounds(player_->transform);
	for (Monster* monster : stressMonsters_)
	{
		if (monster == nullptr || !monster->IsActive() || !monster->collider.enabled)
		{
			continue;
		}

		profiler.Increment(ProfileCounter::CollisionChecks);
		// 결과를 관측 가능한 카운터에 반영해 Release 최적화에서도 AABB 판정이 유지된다.
		if (Intersects(playerBody, monster->GetBodyBox()))
		{
			profiler.Increment(ProfileCounter::StressCollisionOverlaps);
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

void GameWorld::UpdateItemPickup(Profiler& profiler)
{
	// 활성 Item에 대해서만 Player와의 획득 AABB 판정 시간을 측정한다.
	ProfileScope scope(profiler, ProfileCategory::ItemCollision);
	if (!player_)return;
	if(player_->IsDead())return;

	for (WorldItem* item : items_)
	{
		if (!item)continue;
		if(!item->IsActive())continue;

		// 비활성 Item을 제외한 실제 pickup 검사 수를 합산한다.
		profiler.Increment(ProfileCounter::CollisionChecks);
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
