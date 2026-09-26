#pragma once
#include "StatDefinitions.h"

class EnemyStat
{
public:
	explicit EnemyStat(const EnemyStatDefinition& definition) : definition_(definition)
	{
		InitializeForSpawn(1);
	}
	// Spawn 때만 레벨을 확정하므로 살아 있는 적의 보상이 Player 성장에 따라 바뀌지 않는다.
	void InitializeForSpawn(uint32_t level)
	{
		const auto& row = definition_.levels.at(level - 1);
		level_ = level;
		maxHp_ = row.maxHp;
		killExp_ = row.killExp;
	}
	uint32_t GetLevel() const { return level_; }
	uint64_t GetKillExperience() const { return killExp_; }
	int GetMaxHp() const { return maxHp_; }

private:
	const EnemyStatDefinition& definition_;
	uint32_t level_ = 1;
	uint64_t killExp_ = 0;
	int maxHp_ = 3;
};
