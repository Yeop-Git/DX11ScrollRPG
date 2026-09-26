#include "PlayerStat.h"

PlayerStat::PlayerStat(const PlayerStatDefinition& definition) : definition_(definition)
{
	Reset();
}
void PlayerStat::Reset()
{
	level_ = definition_.initialLevel;
	experience_ = definition_.initialExp;
}
uint64_t PlayerStat::GetRequiredExperience() const
{
	return definition_.levels.at(level_ - 1).expToNext;
}
int PlayerStat::GetMaxHp() const
{
	return definition_.levels.at(level_ - 1).maxHp;
}
void PlayerStat::AddExperience(uint64_t reward)
{
	// 큰 보상을 합산하지 않고 필요량씩 소비해 오버플로와 다중 레벨업을 함께 처리한다.
	while (!IsMaxLevel())
	{
		const uint64_t remaining = GetRequiredExperience() - experience_;
		if (reward < remaining)
		{
			experience_ += reward;
			return;
		}
		reward -= remaining;
		++level_;
		experience_ = 0;
	}
	experience_ = 0;
}
