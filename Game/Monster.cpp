#include "Monster.h"
#include <cmath>

Monster::Monster()
{
	maxHp_ = 3;
	hp_ = maxHp_;

	collider.halfSize = { 0.08f, 0.07f };
	transform.position = kStartPosition;

	renderOffsetY = 0.02f;

	idleClip_ = {
		kIdleClipCount,      // frameCount
		0.12f,  // frameDuration
		true,   // loop
		{ 48.0f, 32.0f },
		{ 0.0f, 0.10f }
	};

	chaseClip_ = {
		kChaseClipCount,
		0.10f,
		true,
		{ 48.0f, 32.0f },
		{ 0.0f, 0.10f }
	};

	hurtClip_ = {
		kHurtClipCount,
		0.10f,
		false,
		{ 48.0f, 32.0f },
		{ 0.0f, 0.10f }
	};

	deadClip_ = {
		kDeadClipCount,
		0.12f,
		false,
		{ 48.0f, 32.0f },
		{ 0.0f, 0.10f }
	};

	animator_.Play(idleClip_);
}

void Monster::Update(float deltaTime)
{
	physics.isGrounded = false;

	// animator 갱신
	animator_.Update(deltaTime);

	// 타겟 없으면 
	if (target_ == nullptr) return;
	if (state_ == MonsterState::Dead) return;
	if (target_->IsDead()) return;

	const float targetX = target_->transform.position.x;

	UpdateState(targetX);

	// 넉백 감속
	if (state_ == MonsterState::Hurt)
	{
		physics.velocity.x *= std::pow(0.9f, deltaTime * 60.0f);

		if (animator_.IsFinished())
		{
			physics.velocity.x = 0.0f;
			ChangeState(MonsterState::Idle);
		}
	}
	// Chase 이동
	else if (state_ == MonsterState::Chase)
	{
		if (targetX < transform.position.x)
		{
			physics.velocity.x = -kChaseSpeed;
			facingRight_ = false;
		}
		else
		{
			physics.velocity.x = kChaseSpeed;
			facingRight_ = true;
		}
	}
	else physics.velocity.x = 0.0f;

	ClampWorld();
}

RenderInfo Monster::GetRenderInfo() const
{
	RenderInfo info = Character::GetRenderInfo();

	info.flipX = facingRight_;

	info.visible = true;

	switch (state_)
	{
	case MonsterState::Idle:
		info.spriteId = SpriteId::MonsterIdle;
		break;

	case MonsterState::Chase:
		info.spriteId = SpriteId::MonsterChase;
		break;

	case MonsterState::Hurt:
		info.spriteId = SpriteId::MonsterHurt;
		break;

	case MonsterState::Dead:
		info.spriteId = SpriteId::MonsterDead;
		break;
	}

	return info;
}

void Monster::UpdateState(float playerX)
{
	if (state_ == MonsterState::Hurt) return;
	if (state_ == MonsterState::Dead) return;

	const float distance = std::abs(playerX - transform.position.x);

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

	physics.velocity.x = transform.position.x > attackerX ? kKnockbackSpeed : -kKnockbackSpeed;

	if (hp_ <= 0)
	{
		hp_ = 0;

		physics.velocity.x = 0;

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

	physics.velocity.x = 0.0f;
	ChangeState(MonsterState::Idle);
}

void Monster::Reset()
{
	transform.position = kStartPosition;
	physics.velocity = {};
	hp_ = maxHp_;
	facingRight_ = false;
	ChangeState(MonsterState::Idle);
}

MonsterState Monster::GetState() const
{
	return state_;
}

const Animator& Monster::GetAnimator() const
{
	return animator_;
}
