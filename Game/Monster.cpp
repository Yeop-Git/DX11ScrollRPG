#include "Monster.h"
#include <cmath>

Monster::Monster()
{
	maxHp_ = 3;
	hp_ = maxHp_;

	colliderHalfHeight_ = 0.07f;
	colliderHalfWidth_ = 0.08f;

	x_ = 0.6f;
	y_ = -0.18f;
}

void Monster::Update(float deltaTime)
{
	// 타겟 없으면 
	if (target_ == nullptr) return;
	const float targetX = target_->GetX();

	if (state_ == MonsterState::Dead) return;

	if (state_ == MonsterState::Hit)
	{
		x_ += velocityX_ * deltaTime;
		velocityX_ *= 0.9f;
		return;
	}

	UpdateState(targetX);

	if (state_ == MonsterState::Chase)
	{
		if (targetX < x_)
		{
			velocityX_ = -kMoveSpeed;
			facingRight_ = false;
		}
		else
		{
			velocityX_ = kMoveSpeed;
			facingRight_ = true;
		}

		x_ += velocityX_ * deltaTime;
	}

	else velocityX_ = 0.0f;
}

void Monster::UpdateState(float playerX)
{
	const float distance = std::abs(playerX - x_);

	if (distance <= kChaseRange) state_ = MonsterState::Chase;
	else state_ = MonsterState::Idle;
}

void Monster::TakeDamage(int damage, float attackerX)
{
	if (state_ == MonsterState::Dead) return;
	if (state_ == MonsterState::Hit) return;

	hp_ -= damage;

	velocityX_ = x_ > attackerX ? kKnockbackSpeed : -kKnockbackSpeed;

	if (hp_ <= 0)
	{
		hp_ = 0;
		state_ = MonsterState::Dead;
		return;
	}

	state_ = MonsterState::Hit;
}

void Monster::SetTarget(Character* target)
{
	target_ = target;
}

void Monster::FinishHit()
{
	if (state_ != MonsterState::Hit) return;

	velocityX_ = 0.0f;
	state_ = MonsterState::Idle;
}

void Monster::Reset()
{
	x_ = 0.6f;
	velocityX_ = 0.0f;
	hp_ = 3;
	facingRight_ = false;
	state_ = MonsterState::Idle;
}

AABB Monster::GetHurtBox() const
{
	return
	{
		x_ - colliderHalfWidth_,
		x_ + colliderHalfWidth_,
		y_ - colliderHalfHeight_,
		y_ + colliderHalfHeight_
	};
}

MonsterState Monster::GetState() const
{
	return state_;
}
