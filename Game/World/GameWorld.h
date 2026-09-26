#pragma once

#include <memory>
#include <vector>
#include <array>
#include <queue>
#include <cstddef>
#include <unordered_set>

#include "../../Engine/Core/Profiler.h"
#include "../Entity/Entity.h"
#include "WorldItem.h"

struct GameData;
class Player;
class Monster;
class Ground;
class WorldItem;

enum class RenderLayer : std::size_t
{
	Player,
	Monster,
	Environment,
	Background,
	TransparentItem,
	Count
};

struct RenderObject
{
	GameObject* object = nullptr;
	float depth = 1.0f;
};

class GameWorld
{
public :
	void Initialize(const GameData& data);

	// Application 소유 Profiler를 빌려 Update 하위 구간을 측정한다.
	void Update(float deltaTime, Profiler& profiler);
	void Reset();

	// 지정한 수의 고정 배치 스트레스 Monster를 만들며 0은 테스트 종료다.
	void SetStressTestMonsterCount(std::size_t count);
	std::size_t GetStressTestMonsterCount() const { return stressMonsters_.size(); }
	// 성능 테스트 중에는 충돌 측정은 유지하되 전투 피해와 피격 상태 변경을 막는다.
	void SetCombatDamageEnabled(bool enabled) { combatDamageEnabled_ = enabled; }

	// const reference로 push_back, clear 등으로 수정할 수 없도록
	const std::vector<std::unique_ptr<GameObject>>& GetGameObjects() const
	{
		return gameObjects_;
	}

	const std::array<std::vector<RenderObject>, static_cast<std::size_t>(RenderLayer::Count)>&
	GetRenderLayers() const { return renderLayers_; }

	const std::vector<Entity*>& GetEntities() const
	{
		return entities_;
	}

	Player* GetPlayer() const
	{
		return player_;
	}

private:
	void CreateEnvironment();
	void CreatePlayer();
	void CreateMonsters();
	void CreateGrounds();
	void CreateItems();
	void CreateStressTestMonsters(std::size_t count);
	void ClearStressTestMonsters();
	GameObject* AddGameObject(std::unique_ptr<GameObject> object, RenderLayer layer);
	void RemoveRenderObjects(const std::unordered_set<const GameObject*>& objects);

	void UpdateEntities(float deltaTime, Profiler& profiler);
	void UpdatePhysics(float deltaTime, Profiler& profiler);
	void UpdateCombat(Profiler& profiler);

	// Monster Pool 관리
	void UpdateMonsterLock();
	void CollectDeadMonsters();
	void UpdateMonsterRespawn(float deltaTime);

	// World Item Pool 관리
	WorldItem* FindInactiveItem(ItemType type);
	void DropItem(Vector2 position);
	void DropCoin(Vector2 position);
	void DropPotion(Vector2 position);
	void UpdateItemPickup(Profiler& profiler);

	// Entity - Ground Collision 관리
	void ResolveGroundCollisions(Profiler& profiler);
	void ResolveGroundCollision(Entity& entity, const Ground& ground);

private:
	// Application이 소유하며 모든 일반/풀/스트레스 객체보다 오래 살아 있는 설정.
	const GameData* data_ = nullptr;
	std::vector<std::unique_ptr<GameObject>> gameObjects_;
	// 비소유 렌더 목록은 고정 레이어 순으로 나뉘며 프레임마다 정렬하지 않는다.
	std::array<std::vector<RenderObject>, static_cast<std::size_t>(RenderLayer::Count)> renderLayers_;
	std::vector<Entity*> entities_;
	std::vector<Monster*> monsters_;
	// 일반 Monster의 Unlock / Respawn 풀과 분리한 비소유 스트레스 참조.
	std::vector<Monster*> stressMonsters_;
	// Ground AABB 검사 뒤 스트레스 개체의 위치 보정을 건너뛰기 위한 빠른 참조 집합.
	std::unordered_set<const Entity*> stressMonsterLookup_;
	std::vector<Ground*> grounds_;
	std::vector<WorldItem*> items_;

	// Player는 월드에 하나뿐인 객체임으로 특별 관리.
	Player* player_ = nullptr;
	bool combatDamageEnabled_ = true;

	// Monster Pool
	std::queue<Monster*> respawnQueue_;

	int killCount_ = 0;
	int unlockedMonsterCount_ = 1;

	float respawnTimer_ = 0.0f;
	static constexpr float kRespawnInterval = 2.0f;
	static constexpr int kMonsterUnlockKills[] = { 0,3,7 };

	// Item Pool
	std::queue<WorldItem*> coinPool_;
	std::queue<WorldItem*> potionPool_;

	static constexpr int kCoinPoolSize = 5;
	static constexpr int kPotionPoolSize = 3;

	static constexpr int kPotionRate = 50;

	// Physics 설정 값
	static constexpr float gravity_ = -3.0f;

	// Ground, Tree 값
	static constexpr float groundTop = -0.3f;
	static constexpr Vector2 groundHalfSize{ 0.1f, 0.1f };
	static constexpr Vector2 firstGroundPosition{-1.0f, groundTop - groundHalfSize.y};
	static constexpr int groundCount = 15;

	static constexpr Vector2 treeHalfSize{ 0.1f, 0.34f };
	static constexpr float treeCenterY = groundTop + treeHalfSize.y;
};
