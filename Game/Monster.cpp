#include "Monster.h"
#include <cmath>

Monster::Monster(const MonsterDefinition& definition)
	: definition_(definition)
{
	// 검증된 공유 정의를 사용하고, 현재 HP와 Animator 재생 상태만 객체별로 가진다.
	maxHp_ = definition_.maxHp;
	hp_ = maxHp_;
	renderOffsetY = definition_.renderOffsetY;
	collider.halfSize = definition_.colliderHalfSize;
	transform.position = definition_.startPosition;
	animator_.Play(definition_.idle);
}

void Monster::Update(float deltaTime)
{
	physics.isGrounded = false;

	// animator 갱신
	animator_.Update(deltaTime);

	// 타겟 없으면 
	if (target_ == nullptr) return;
	if (state_ == MonsterState::Dead) return;

	UpdateState();

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
		if (target_->transform.position.x < transform.position.x)
		{
			physics.velocity.x = -definition_.chaseSpeed;
			facingRight_ = false;
		}
		else
		{
			physics.velocity.x = definition_.chaseSpeed;
			facingRight_ = true;
		}
	}
	else physics.velocity.x = 0.0f;

	ClampWorld();
}

void Monster::OnEnable()
{
	hp_ = maxHp_;

	physics.enabled = true;
	physics.velocity = {};
	physics.isGrounded = false;

	collider.enabled = true;

	facingRight_ = false;

	transform.position = definition_.startPosition;

	ChangeState(MonsterState::Idle);
}

void Monster::OnDisable()
{
	physics.velocity = {};
	collider.enabled = false;
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

void Monster::UpdateState()
{
	if (state_ == MonsterState::Hurt) return;
	if (state_ == MonsterState::Dead) return;

	const float distance = std::abs(target_->transform.position.x - transform.position.x);

	if (distance <= definition_.chaseRange && !target_->IsDead()) ChangeState(MonsterState::Chase);
	else ChangeState(MonsterState::Idle);
}

void Monster::ChangeState(MonsterState newState)
{
	if (state_ == newState) return;

	state_ = newState;

	switch (state_)
	{
	case MonsterState::Idle:
		animator_.Play(definition_.idle);
		break;
	case MonsterState::Chase:
		animator_.Play(definition_.chase);
		break;
	case MonsterState::Hurt:
		animator_.Play(definition_.hurt);
		break;
	case MonsterState::Dead:
		animator_.Play(definition_.dead);
		break;
	}
}

void Monster::TakeDamage(int damage, float attackerX)
{
	if (state_ == MonsterState::Dead) return;
	if (state_ == MonsterState::Hurt) return;

	hp_ -= damage;

	physics.velocity.x = transform.position.x > attackerX ? definition_.knockbackSpeed : -definition_.knockbackSpeed;

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
	transform.position = definition_.startPosition;
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

bool Monster::IsDeadAnimationFinished() const
{
	return state_ == MonsterState::Dead && animator_.IsFinished();
}