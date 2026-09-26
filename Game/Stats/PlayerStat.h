#pragma once
#include "StatDefinitions.h"

class PlayerStat
{
public:
	explicit PlayerStat(const PlayerStatDefinition& definition);
	void AddExperience(uint64_t reward);
	void Reset();
	uint32_t GetLevel() const { return level_; }
	uint64_t GetExperience() const { return experience_; }
	uint64_t GetRequiredExperience() const;
	int GetMaxHp() const;
	bool IsMaxLevel() const { return level_ == definition_.levels.size(); }

private:
	const PlayerStatDefinition& definition_;
	uint32_t level_ = 1;
	uint64_t experience_ = 0;
};
