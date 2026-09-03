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

	idleClip_ = {
		kIdleClipCount,      // frameCount
		0.12f,  // frameDuration
		true,   // loop
		32, 32,
		0.10f
	};

	chaseClip_ = {
		kChaseClipCount,
		0.10f,
		true,
		32, 32,
		0.10f
	};

	hurtClip_ = {
		kHurtClipCount,
		0.10f,
		false,
		32, 32,
		0.10f
	};

	deadClip_ = {
		kDeadClipCount,
		0.12f,
		false,
		32, 32,
		0.10f
	};

	animator_.Play(idleClip_);
}

void Monster::Update(float deltaTime)
{
	// animator 갱신
	animator_.Update(deltaTime);

	// 타겟 없으면 
	if (target_ == nullptr) return;
	if (state_ == MonsterState::Dead) return;

	const float targetX = target_->GetX();

	UpdateState(targetX);

	// 넉백 감속
	if (state_ == MonsterState::Hurt)
	{
		velocityX_ *= std::pow(0.9f, deltaTime * 60.0f);

		if (animator_.IsFinished())
		{
			velocityX_ = 0.0f;
			ChangeState(MonsterState::Idle);
		}
	}
	// Chase 이동
	else if (state_ == MonsterState::Chase)
	{
		if (targetX < x_)
		{
			velocityX_ = -kChaseSpeed;
			facingRight_ = false;
		}
		else
		{
			velocityX_ = kChaseSpeed;
			facingRight_ = true;
		}
	}
	else velocityX_ = 0.0f;

	UpdatePosition(deltaTime);
}

void Monster::UpdateState(float playerX)
{
	if (state_ == MonsterState::Hurt) return;
	if (state_ == MonsterState::Dead) return;

	const float distance = std::abs(playerX - x_);

	if (distance <= kChaseRange) ChangeState(MonsterState::Chase);
	else ChangeState(MonsterState::Idle);
}

void Monster::ChangeState(MonsterState newState)
{
	if (state_ == newState) return;

	state_ = newState;

	switch (state_)
	{
	case MonsterState::Idle:
		animator_.Play(idleClip_);
		break;
	case MonsterState::Chase:
		animator_.Play(chaseClip_);
		break;
	case MonsterState::Hurt:
		animator_.Play(hurtClip_);
		break;
	case MonsterState::Dead:
		animator_.Play(deadClip_);
		break;
	}
}

void Monster::TakeDamage(int damage, float attackerX)
{
	if (state_ == MonsterState::Dead) return;
	if (state_ == MonsterState::Hurt) return;

	hp_ -= damage;

	velocityX_ = x_ > attackerX ? kKnockbackSpeed : -kKnockbackSpeed;

	if (hp_ <= 0)
	{
		hp_ = 0;
		ChangeState(MonsterState::Dead);
		return;
	}

	ChangeState(MonsterState::Hurt);
}

void Monster::SetTarget(Character* target)
{
	target_ = target;
}

void Monster::FinishHit()
{
	if (state_ != MonsterState::Hurt) return;

	velocityX_ = 0.0f;
	ChangeState(MonsterState::Idle);
}

void Monster::Reset()
{
	x_ = 0.6f;
	velocityX_ = 0.0f;
	hp_ = maxHp_;
	facingRight_ = false;
	ChangeState(MonsterState::Idle);
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

const Animator& Monster::GetAnimator() const
{
	return animator_;
}
