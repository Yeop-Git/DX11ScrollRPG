#include "Player.h"

#include <Windows.h>
#include <algorithm>
#include <cmath>

Player::Player()
{
	maxHp_ = 3;
	hp_ = maxHp_;

	renderOffsetY = -0.05f;

	idleClip_ = {
		kIdleClipCount,      // frameCount
		0.12f,  // frameDuration
		true,   // loop
		64, 80,
		0.22f
	};

	runClip_ = {
		kRunClipCount,
		0.10f,
		true,
		80, 80,
		0.22f
	};

	jumpStartClip_ = {
		kJumpStartClipCount,
		0.10f,
		false,
		64, 64,
		0.176f
	};

	jumpEndClip_ = {
		kJumpEndClipCount,
		0.10f,
		false,
		64, 64,
		0.176f
	};

	attackClip_ = {
		kAttackClipCount,
		0.08f,
		false,
		96, 80,
		0.22f
	};

	deadClip_ = {
		kDeadClipCount,
		0.12f,
		false,
		80, 64,
		0.176f
	};

	animator_.Play(idleClip_);

	collider.halfWidth = 0.1f;
	collider.halfHeight = 0.18f;

	transform.x = 0.0f;
	transform.y = kGroundY + collider.halfHeight;
}

void Player::Update(float deltaTime)
{
	animator_.Update(deltaTime);

	UpdateDamageState(deltaTime);

	HandleInput();

	ApplyGravity(deltaTime);

	// Update Position 후 ResolveGroundCollisions 순서를 지켜야 점프가 가능
	UpdatePosition(deltaTime);

	ResolveGroundCollisions();

	if (state_ == PlayerState::Attack)
	{
		if (animator_.IsFinished()) FinishAttack();
	}

	UpdateState();
}

RenderInfo Player::GetRenderInfo() const
{
	RenderInfo info = Character::GetRenderInfo();

	info.flipX = !facingRight_;
	info.visible = ShouldRender();

	switch (state_)
	{
	case PlayerState::Idle:
		info.spriteId = SpriteId::PlayerIdle;
		break;

	case PlayerState::Run:
		info.spriteId = SpriteId::PlayerRun;
		break;

	case PlayerState::JumpStart:
		info.spriteId =
			SpriteId::PlayerJumpStart;
		break;

	case PlayerState::JumpEnd:
		info.spriteId =
			SpriteId::PlayerJumpEnd;
		break;

	case PlayerState::Attack:
		info.spriteId =
			SpriteId::PlayerAttack;
		break;

	case PlayerState::Dead:
		info.spriteId =
			SpriteId::PlayerDead;
		break;
	}

	return info;
}

void Player::HandleInput()
{
	if (state_ == PlayerState::Dead) return;
	if (knockbackTimer_ > 0.0f) return;

	velocityX_ = 0.0f;

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

void Player::ResolveGroundCollisions()
{
	// 땅과 충돌 처리
	if (transform.y <= kGroundY + collider.halfHeight)
	{
		transform.y = kGroundY + collider.halfHeight;
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
		if (velocityY_ > 0.0f)
		{
			ChangeState(PlayerState::JumpStart);
		}
		else
		{
			ChangeState(PlayerState::JumpEnd);
		}
	}
	else if (velocityX_ != 0.0f)
	{
		ChangeState(PlayerState::Run);
	}
	else
	{
		ChangeState(PlayerState::Idle);
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
		velocityX_ *= std::pow(0.9f, deltaTime * 60.0f);
	}

	if (invincibleTimer_ <= 0.0f)
	{
		invincibleTimer_ = 0.0f;
		isInvincible_ = false;
	}

	if (knockbackTimer_ < 0.0f)
		knockbackTimer_ = 0.0f;
}

void Player::ChangeState(PlayerState newState)
{
	if (state_ == newState) return;
	state_ = newState;

	switch (state_)
	{
	case PlayerState::Idle:
		animator_.Play(idleClip_);
		break;

	case PlayerState::Run:
		animator_.Play(runClip_);
		break;

	case PlayerState::JumpStart:
		animator_.Play(jumpStartClip_);
		break;

	case PlayerState::JumpEnd:
		animator_.Play(jumpEndClip_);
		break;

	case PlayerState::Attack:
		animator_.Play(attackClip_);
		break;

	case PlayerState::Dead:
		animator_.Play(deadClip_);
		break;
	}
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

		ChangeState(PlayerState::Dead);
		return;
	}

	// 무적 시작
	isInvincible_ = true;
	invincibleTimer_ = kInvincibleDuration;

	// 넉백 시작
	knockbackTimer_ = kKnockbackDuration;

	velocityX_ = transform.x < attackerX ? -kKnockbackSpeedX : kKnockbackSpeedX;

	velocityY_ = kKnockbackSpeedY;
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
	ChangeState(PlayerState::Attack);
	attackHitRegistered_ = false;
	velocityX_ = 0.0f;
}

void Player::FinishAttack()
{
	if (state_ == PlayerState::Attack) ChangeState(PlayerState::Idle);
}

void Player::Reset()
{
	transform.x = 0.0f;
	transform.y = kGroundY + collider.halfHeight;

	velocityX_ = 0.0f;
	velocityY_ = 0.0f;

	hp_ = maxHp_;

	isGrounded_ = true;
	facingRight_ = true;

	isInvincible_ = false;
	invincibleTimer_ = 0.0f;
	knockbackTimer_ = 0.0f;
	attackHitRegistered_ = false;

	ChangeState(PlayerState::Idle);
}

bool Player::IsAttackFrameActive() const
{
	if (state_ != PlayerState::Attack) return false;

	const int frame = animator_.GetCurrentFrame();
	return frame >= 2 && frame <= 3;
}

bool Player::CanRegisterAttackHit() const
{
	return IsAttackFrameActive() && !attackHitRegistered_;
}

void Player::RegisterAttackHit()
{
	attackHitRegistered_ = true;
}

AABB Player::GetAttackHitBox() const
{
	constexpr float attackWidth = 0.18f;
	constexpr float attackHalfHeight = 0.10f;

	if (facingRight_)
	{
		return
		{
			transform.x,
			transform.x + attackWidth,
			transform.y - attackHalfHeight,
			transform.y + attackHalfHeight
		};
	}

	return
	{
		transform.x - attackWidth,
		transform.x,
		transform.y - attackHalfHeight,
		transform.y + attackHalfHeight
	};
}

PlayerState Player::GetState() const
{
	return state_;
}

const Animator& Player::GetAnimator() const
{
	return animator_;
}