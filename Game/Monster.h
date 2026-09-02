#pragma once

#include "Collision.h"

enum class MonsterState
{
	Idle,
	Chase,
	Hit,
	Dead
};

class Monster
{
public :
	void Update(float deltaTime, float playerX);

	void TakeDamage(int damage, float attackerX);
	void FinishHit();
	void Reset();

	float GetX() const;
	float GetY() const;

	int GetHP() const;

	bool IsFacingRight() const;
	bool IsDead() const;

	MonsterState GetState() const;

	AABB GetHurtBox() const;
	AABB GetBodyBox() const;

private:
	void UpdateState(float playerX);

private :
	float x_ = 0.6f;
	float y_ = -0.18f;

	float velocityX_ = 0.0f;

	int hp_ = 3;
	int maxHp_ = 3;

	bool facingRight_ = false;

	MonsterState state_ = MonsterState::Idle;

	static constexpr float kMoveSpeed = 0.25f;
	static constexpr float kChaseRange = 0.6f;

	static constexpr float kKnockbackSpeed = 0.45f;
};