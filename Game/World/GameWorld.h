#pragma once

#include <memory>
#include <vector>
#include <queue>

#include "../Entity/Entity.h"

class Player;
class Monster;
class Ground;

class GameWorld
{
public :
	void Initialize();

	void Update(float deltaTime);
	void Reset();

	// const reference로 push_back, clear 등으로 수정할 수 없도록
	const std::vector<std::unique_ptr<GameObject>>& GetGameObjects() const
	{
		return gameObjects_;
	}

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

	void UpdateEntities(float deltaTime);
	void UpdatePhysics(float deltaTime);
	void UpdateCombat();

	void UpdateMonsterLock();
	void CollectDeadMonsters();
	void UpdateMonsterRespawn(float deltaTime);


	void ResolveGroundCollisions();
	void ResolveGroundCollision(Entity& entity, const Ground& ground);

private:
	std::vector<std::unique_ptr<GameObject>> gameObjects_;
	std::vector<Entity*> entities_;
	std::vector<Monster*> monsters_;
	std::vector<Ground*> grounds_;

	// Player는 월드에 하나뿐인 객체임으로 특별 관리.
	Player* player_ = nullptr;

	// Monster Spawner
	std::queue<Monster*> respawnQueue_;

	int killCount_ = 0;
	int unlockedMonsterCount_ = 1;

	float respawnTimer_ = 0.0f;
	static constexpr float kRespawnInterval = 2.0f;
	static constexpr int kMonsterUnlockKills[] = { 0,3,7 };

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
