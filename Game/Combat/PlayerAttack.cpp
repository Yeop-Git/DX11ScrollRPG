#include "PlayerAttack.h"
#include "../Player.h"
#include "../World/GameWorld.h"
#include <algorithm>

AttackAvailability PlayerAttack::GetAvailability(const Player& player) const
{
	if (player.GetStat().GetLevel() < definition_.requiredLevel) return AttackAvailability::Locked;
	const auto state = player.GetAttackState();
	if (state != AttackAvailability::Ready) return state;
	return remaining_ > 0.0f ? AttackAvailability::Cooldown : AttackAvailability::Ready;
}
AttackAvailability PlayerAttack::TryUse(Player& player, GameWorld& world)
{
	const auto available = GetAvailability(player);
	if (available != AttackAvailability::Ready) return available;
	const auto result = Execute(player, world);
	if (result == AttackAvailability::Ready) remaining_ = definition_.cooldown;
	return result;
}
void PlayerAttack::UpdateCooldown(float dt)
{
	remaining_ = (std::max)(0.0f, remaining_ - dt);
}
AttackAvailability MeleeAttack::Execute(Player& player, GameWorld&)
{
	player.StartAttack();
	return AttackAvailability::Ready;
}
AttackAvailability ProjectileAttack::Execute(Player& player, GameWorld& world)
{
	return world.SpawnSkill(slot_, player);
}
AttackAvailability GroundAreaAttack::Execute(Player& player, GameWorld& world)
{
	return world.SpawnSkill(AttackSlot::E, player);
}
AttackAvailability PeriodicAttack::Execute(Player& player, GameWorld& world)
{
	return world.SpawnSkill(AttackSlot::R, player);
}
