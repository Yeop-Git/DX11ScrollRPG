#include "Monster.h"
#include <cmath>

void Monster::Update(float deltaTime, float playerX)
{
	if (state_ == MonsterState::Dead || state_ == MonsterState::Hit) return;

	UpdateState(playerX);

	if (state_ == MonsterState::Chase)
	{
		if (playerX < x_)
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

void Monster::TakeDamage(int damage)
{
	if (state_ == MonsterState::Dead) return;

	hp_ -= damage;

	velocityX_ = 0.0f;

	if (hp_ <= 0)
	{
		hp_ = 0;
		state_ = MonsterState::Dead;
		return;
	}

	state_ = MonsterState::Hit;
}

void Monster::FinishHit()
{
	if (state_ == MonsterState::Hit)
	{
		state_ == MonsterState::Idle;
	}
}

float Monster::GetX() const
{
	return x_;
}


float Monster::GetY() const
{
	return y_;
}

int Monster::GetHP() const
{
	return hp_;
}

bool Monster::IsFacingRight() const
{
	return facingRight_;
}

bool Monster::IsDead() const
{
	return state_ == MonsterState::Dead;
}

MonsterState Monster::GetState() const
{
	return state_;
}
