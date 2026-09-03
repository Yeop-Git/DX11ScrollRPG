#pragma once

#include "Entity/Character.h"

enum class MonsterState
{
	Idle,
	Chase,
	Hit,
	Dead
};

class Monster : public Character
{
public :
	Monster();

	// Update에서 playerX 제거
	void Update(float deltaTime) override;

	void TakeDamage(int damage, float attackerX);

	// Player를 Target, Character 포인터로 받음
	void SetTarget(Character* target);

	void FinishHit();
	void Reset();

	MonsterState GetState() const;

	AABB GetHurtBox() const;

private:
	void UpdateState(float playerX);

private :
	Character* target_ = nullptr;
	MonsterState state_ = MonsterState::Idle;

	static constexpr float kMoveSpeed = 0.25f;
	static constexpr float kChaseRange = 0.6f;

	static constexpr float kKnockbackSpeed = 0.45f;
};