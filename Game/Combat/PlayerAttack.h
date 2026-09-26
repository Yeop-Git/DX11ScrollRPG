#pragma once
#include "AttackDefinition.h"
class Player;
class GameWorld;

class PlayerAttack
{
public:
	explicit PlayerAttack(const AttackDefinition& definition) : definition_(definition) {}
	virtual ~PlayerAttack() = default;
	AttackAvailability GetAvailability(const Player& player) const;
	AttackAvailability TryUse(Player& player, GameWorld& world);
	void UpdateCooldown(float dt);
	void Reset() { remaining_ = 0.0f; }
	float GetRemainingCooldown() const { return remaining_; }
	const AttackDefinition& GetDefinition() const { return definition_; }

protected:
	virtual AttackAvailability Execute(Player& player, GameWorld& world) = 0;
	const AttackDefinition& definition_;

private:
	float remaining_ = 0.0f;
};
class MeleeAttack final : public PlayerAttack
{
public:
	using PlayerAttack::PlayerAttack;

private:
	AttackAvailability Execute(Player& player, GameWorld& world) override;
};
// Q와 W는 설정만 다르므로 같은 투사체 행동을 조합한다.
class ProjectileAttack final : public PlayerAttack
{
public:
	ProjectileAttack(const AttackDefinition& definition, AttackSlot slot)
		: PlayerAttack(definition), slot_(slot)
	{
	}

private:
	AttackAvailability Execute(Player& player, GameWorld& world) override;
	AttackSlot slot_;
};
class GroundAreaAttack final : public PlayerAttack
{
public:
	using PlayerAttack::PlayerAttack;

private:
	AttackAvailability Execute(Player& player, GameWorld& world) override;
};
class PeriodicAttack final : public PlayerAttack
{
public:
	using PlayerAttack::PlayerAttack;

private:
	AttackAvailability Execute(Player& player, GameWorld& world) override;
};
