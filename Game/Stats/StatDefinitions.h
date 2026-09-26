#pragma once
#include <cstdint>
#include <vector>

// GameData가 소유하는 불변 레벨 테이블. 현재 HP는 Character만 소유한다.
struct PlayerLevelDefinition
{
	uint64_t expToNext = 0;
	int maxHp = 3;
};
struct EnemyLevelDefinition
{
	uint64_t killExp = 0;
	int maxHp = 3;
};
struct PlayerStatDefinition
{
	uint32_t initialLevel = 1;
	uint64_t initialExp = 0;
	std::vector<PlayerLevelDefinition> levels;
};
struct EnemyStatDefinition
{
	std::vector<EnemyLevelDefinition> levels;
};
