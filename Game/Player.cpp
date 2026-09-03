#include "Player.h"

#include <Windows.h>
#include <algorithm>

Player::Player()
{
	maxHp_ = 3;
	hp_ = maxHp_;

	colliderHalfWidth_ = 0.1f;
	colliderHalfHeight_ = 0.18f;

	x_ = 0.0f;
	y_ = kGroundY + colliderHalfHeight_;
}

void Player::Update(float deltaTime)
{
	// Input > Physics > Position > Collision > State
	UpdateDamageState(deltaTime);

	HandleInput();

	if (state_ == PlayerState::Attack) return;

	ApplyGravity(deltaTime);

	UpdatePosition(deltaTime);

	ResolveGroundCollisions();

	UpdateState();
}

void Player::HandleInput()
{
	velocityX_ = 0.0f;

	if (state_ == PlayerState::Dead) return;
	if (knockbackTimer_ > 0.0f) return;

	// Attack
	if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
	{
		StartAttack();
	}

	if (state_ == PlayerState::Attack) return;

	// 화살표 수평 이동 처리
	if (GetAsyncKeyState(VK_LEFT) & 0x8000)
	{
		velocityX_ = -kMoveSpeed;
		facingRight_ = false;
	}

	if (GetAsyncKeyState(VK_RIGHT) & 0x8000)
	{
		velocityX_ = kMoveSpeed;
		facingRight_ = true;
	}


	// Alt 점프 처리
	if (isGrounded_ && GetAsyncKeyState(VK_MENU) & 0x8000)
	{
		velocityY_ = kJumpSpeed;
		isGrounded_ = false;
	}
}

void Player::ApplyGravity(float deltaTime)
{
	// 공중에서는 중력 적용
	if (!isGrounded_)
	{
		velocityY_ += kGravity * deltaTime;
	}
}

void Player::UpdatePosition(float deltaTime)
{
	// Velocity에 따른 위치 업데이트
	x_ += velocityX_ * deltaTime;
	y_ += velocityY_ * deltaTime;

	// 화면 밖으로 나가지 않도록 clamp
	x_ = std::clamp(x_, -1.0f + colliderHalfWidth_, 1.0f - colliderHalfWidth_);
}

void Player::ResolveGroundCollisions()
{
	// 땅과 충돌 처리
	if (y_ <= kGroundY + colliderHalfHeight_)
	{
		y_ = kGroundY + colliderHalfHeight_;
		velocityY_ = 0.0f;
		isGrounded_ = true;
	}
}

void Player::UpdateState()
{
	if (state_ == PlayerState::Attack) return;
	if (state_ == PlayerState::Dead) return;

	// 상태 전환
	if (!isGrounded_)
	{
		state_ = velocityY_ > 0.0f ? PlayerState::JumpStart : PlayerState::JumpEnd;
	}
	else if (velocityX_ != 0.0f)
	{
		state_ = PlayerState::Run;
	}
	else
	{
		state_ = PlayerState::Idle;
	}
}

void Player::UpdateDamageState(float deltaTime)
{
	if (invincibleTimer_ > 0.0f)
	{
		invincibleTimer_ -= deltaTime;
	}
	if (knockbackTimer_ > 0.0f)
	{
		knockbackTimer_ -= deltaTime;
	}

	if (invincibleTimer_ <= 0.0f)
	{
		invincibleTimer_ = 0.0f;
		isInvincible_ = false;
	}

	if (knockbackTimer_ < 0.0f)
		knockbackTimer_ = 0.0f;
}

void Player::TakeDamage(int damage, float attackerX)
{
	if (state_ == PlayerState::Dead) return;
	if (isInvincible_) return;;

	hp_ -= damage;


	if (hp_ <= 0)
	{
		hp_ = 0;

		velocityX_ = 0.0f;
		velocityY_ = 0.0f;

		state_ = PlayerState::Dead;
		return;
	}

	// 무적 시작
	isInvincible_ = true;
	invincibleTimer_ = kInvincibleDuration;

	// 넉백 시작
	knockbackTimer_ = kKnockbackDuration;

	velocityX_ = x_ < attackerX ? -kKnockbackSpeed : kKnockbackSpeed;

	velocityY_ = 0.5f;
	isGrounded_ = false;
}

bool Player::ShouldRender() const
{
	if (!isInvincible_) return true;
	constexpr float blinkInterval = 0.1f;

	const int blinkPhase = static_cast<int>(invincibleTimer_ / blinkInterval);

	return blinkPhase % 2 == 0;
}

void Player::StartAttack()
{
	if (state_ == PlayerState::Attack) return;
	if (state_ == PlayerState::Dead) return;
	if (!isGrounded_) return;
	state_ = PlayerState::Attack;
	velocityX_ = 0.0f;
}

void Player::FinishAttack()
{
	if (state_ == PlayerState::Attack) state_ = PlayerState::Idle;
}

void Player::Reset()
{
	x_ = 0.0f;
	y_ = kGroundY + colliderHalfHeight_;

	velocityX_ = 0.0f;
	velocityY_ = 0.0f;

	hp_ = maxHp_;

	isGrounded_ = true;
	facingRight_ = true;

	state_ = PlayerState::Idle;
}

bool Player::IsAttacking() const
{
	return state_ == PlayerState::Attack;
}

AABB Player::GetAttackHitBox() const
{
	constexpr float attackWidth = 0.18f;
	constexpr float attackHalfHeight = 0.10f;

	if (facingRight_)
	{
		return
		{
			x_,
			x_ + attackWidth,
			y_ - attackHalfHeight,
			y_ + attackHalfHeight
		};
	}

	return
	{
		x_ - attackWidth,
		x_,
		y_ - attackHalfHeight,
		y_ + attackHalfHeight
	};
}

PlayerState Player::GetState() const
{
	return state_;
}