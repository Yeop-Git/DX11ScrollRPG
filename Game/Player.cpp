#include "Player.h"

#include <Windows.h>
#include <algorithm>

void Player::Update(float deltaTime)
{
	// Input > Physics > Position > Collision > State
	HandleInput();

	ApplyGravity(deltaTime);

	UpdatePosition(deltaTime);

	ResolveGroundCollisions();

	UpdateState();
}

void Player::HandleInput()
{
	velocityX_ = 0.0f;

	// A/D 수평 이동 처리
	if (GetAsyncKeyState('A') & 0x8000)
	{
		velocityX_ = -kMoveSpeed;
		facingRight_ = false;

	}

	if (GetAsyncKeyState('D') & 0x8000)
	{
		velocityX_ = kMoveSpeed;
		facingRight_ = true;
	}

	// Space 점프 처리
	if(isGrounded_ && GetAsyncKeyState(VK_SPACE) & 0x8000)
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
	x_ = std::clamp(x_, -1.0f + kHalfWidth, 1.0f - kHalfWidth);
}

void Player::ResolveGroundCollisions()
{
	// 땅과 충돌 처리
	if (y_ <= kGroundY + kHalfHeight)
	{
		y_ = kGroundY + kHalfHeight;
		velocityY_ = 0.0f;
		isGrounded_ = true;
	}
}

void Player::UpdateState()
{
	// 상태 전환
	if (!isGrounded_)
	{
		state_ = PlayerState::Jump;
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

// Getter
float Player::GetX() const
{
	return x_;
}

float Player::GetY() const
{
	return y_;
}

bool Player::IsFacingRight() const
{
	return facingRight_;
}

PlayerState Player::GetState() const
{
	return state_;
}