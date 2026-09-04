#pragma once

#include <memory>
#include <vector>

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
	void CreateEntities();
	void CreateGrounds();

	void UpdateEntities(float deltaTime);
	void ResolveGroundCollisions();
	void ResolveGroundCollision(Entity& entity, const Ground& ground);
	void UpdateCombat();

private:
	std::vector<std::unique_ptr<GameObject>> gameObjects_;
	std::vector<Entity*> entities_;
	std::vector<Ground*> grounds_;

	// Player는 월드에 하나뿐인 객체임으로 특별 관리.
	Player* player_ = nullptr;

	static constexpr float groundTop = -0.3f;
	static constexpr Vector2 groundHalfSize{ 0.1f, 0.1f };
	static constexpr Vector2 firstGroundPosition{
		-1.0f,
		groundTop - groundHalfSize.y
	};
	static constexpr int groundCount = 15;

	static constexpr Vector2 treeHalfSize{ 0.1f, 0.34f };
	static constexpr float treeCenterY = groundTop + treeHalfSize.y;
};
