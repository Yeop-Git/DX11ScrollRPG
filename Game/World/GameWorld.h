#pragma once

#include <memory>
#include <vector>

#include "../Entity//Entity.h"

class Player;
class Monster;

class GameWorld
{
public :
	void Initialize();

	void Update(float deltaTime);
	void Reset();

	Player* GetPlayer() const
	{
		return player_;
	}

	Monster* GetMonster() const
	{
		return monster_;
	}

private:
	void CreateEntities();
	void UpdateEntities(float deltaTime);
	void UpdateCombat();

private:
	std::vector<std::unique_ptr<Entity>> entities_;

	// Player는 월드에 하나뿐인 객체임으로 특별 관리.
	Player* player_ = nullptr;
	// 현재는 몬스터 하나만 임시 관리.
	Monster* monster_ = nullptr;
};