#pragma once

#include "Entity.h"
#include "../Animation/Animator.h"

// 움직이며 전투가능한 Entity
class Character : public Entity
{
public:
	// 소멸자 virtual
	virtual ~Character() = default;

	// Take Damage virtual로 선언하여 다형성 부여
	virtual void TakeDamage(int damage, float attackerX);

	void UpdatePosition(float deltaTime);

	RenderInfo GetRenderInfo() const override;

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

protected:
	Animator animator_;

	// 전투가 가능한 Entity이기 때문에 HP, Collider, Velocity를 가짐.
	int hp_ = 3;
	int maxHp_ = 3;

	bool facingRight_ = true;

	float renderOffsetY = -0.07f;
};
