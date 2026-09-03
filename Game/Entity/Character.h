#pragma once

#include "Entity.h"
#include "../Collision/AABB.h"

// 움직이며 전투가능한 Entity
class Character : public Entity
{
public:
	// 소멸자 virtual
	virtual ~Character() = default;

	// Take Damage virtual로 선언하여 다형성 부여
	virtual void TakeDamage(int damage, float attackerX);

	int GetHP() const
	{
		return hp_;
	}

	int GetMaxHP() const
	{
		return maxHp_;
	}

	bool IsDead() const
	{
		return hp_ <= 0;
	}

	bool IsFacingRight() const
	{
		return facingRight_;
	}

	AABB GetBodyBox() const;

protected:
	// 전투가 가능한 Entity이기 때문에 HP, Collider, Velocity를 가짐.
	int hp_ = 3;
	int maxHp_ = 3;

	float velocityX_ = 0.0f;
	float velocityY_ = 0.0f;

	bool facingRight_ = true;

	float colliderHalfWidth_ = 0.05f;
	float colliderHalfHeight_ = 0.05f;
};