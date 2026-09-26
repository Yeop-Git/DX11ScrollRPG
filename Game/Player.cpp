#include "Player.h"

#include <Windows.h>
#include <algorithm>
#include <cmath>

Player::Player(const PlayerDefinition& definition)
	: definition_(definition)
{
	// 검증된 공유 정의를 사용하고, 현재 HP와 Animator 재생 상태만 객체별로 가진다.
	maxHp_ = definition_.maxHp;
	hp_ = maxHp_;
	renderOffsetY = definition_.renderOffsetY;
	collider.halfSize = definition_.colliderHalfSize;
	transform.position = definition_.startPosition;
	animator_.Play(definition_.idle);
	physics.isGrounded = true;
}

void Player::Update(float deltaTime)
{
	const bool wasGrounded = physics.isGrounded;

	animator_.Update(deltaTime);

	UpdateDamageState(deltaTime);

	HandleInput();


	ClampWorld();

	if (state_ == PlayerState::Attack)
	{
		if (animator_.IsFinished()) FinishAttack();
	}

	UpdateState(wasGrounded);
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

	physics.velocity.x = 0.0f;

	// Attack
	if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
	{
		StartAttack();
	}

	if (state_ == PlayerState::Attack) return;

	// 화살표 수평 이동 처리
	if (GetAsyncKeyState(VK_LEFT) & 0x8000)
	{
		physics.velocity.x = -definition_.moveSpeed;
		facingRight_ = false;
	}

	if (GetAsyncKeyState(VK_RIGHT) & 0x8000)
	{
		physics.velocity.x = definition_.moveSpeed;
		facingRight_ = true;
	}


	// Alt 점프 처리
	if (physics.isGrounded && GetAsyncKeyState(VK_MENU) & 0x8000)
	{
		physics.velocity.y = definition_.jumpSpeed;
		physics.isGrounded = false;
	}
}

void Player::UpdateState(bool wasGrounded)
{
	if (state_ == PlayerState::Attack) return;
	if (state_ == PlayerState::Dead) return;

	const bool isGroundedForState =
		wasGrounded && physics.velocity.y <= 0.0f;

	// 상태 전환
	if (!isGroundedForState)
	{
		if (physics.velocity.y > 0.0f)
		{
			ChangeState(PlayerState::JumpStart);
		}
		else
		{
			ChangeState(PlayerState::JumpEnd);
		}
	}
	else if (physics.velocity.x != 0.0f)
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
		physics.velocity.x *= std::pow(0.9f, deltaTime * 60.0f);
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
		animator_.Play(definition_.idle);
		break;

	case PlayerState::Run:
		animator_.Play(definition_.run);
		break;

	case PlayerState::JumpStart:
		animator_.Play(definition_.jumpStart);
		break;

	case PlayerState::JumpEnd:
		animator_.Play(definition_.jumpEnd);
		break;

	case PlayerState::Attack:
		animator_.Play(definition_.attack);
		break;

	case PlayerState::Dead:
		animator_.Play(definition_.dead);
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

		physics.velocity = {};

		ChangeState(PlayerState::Dead);
		return;
	}

	// 무적 시작
	isInvincible_ = true;
	invincibleTimer_ = definition_.invincibleDuration;

	// 넉백 시작
	knockbackTimer_ = definition_.knockbackDuration;

	physics.velocity = definition_.knockbackSpeed;
	if (transform.position.x < attackerX) physics.velocity.x *= -1.0f;
	physics.isGrounded = false;
}

bool Player::ShouldRender() const
{
	if (!isInvincible_) return true;
	const float blinkInterval = definition_.blinkInterval;

	const int blinkPhase = static_cast<int>(invincibleTimer_ / blinkInterval);

	return blinkPhase % 2 == 0;
}

void Player::StartAttack()
{
	if (state_ == PlayerState::Attack) return;
	if (state_ == PlayerState::Dead) return;
	if (!physics.isGrounded) return;

	ChangeState(PlayerState::Attack);
	attackHitRegistered_ = false;
	physics.velocity.x = 0.0f;
}

void Player::FinishAttack()
{
	if (state_ == PlayerState::Attack) ChangeState(PlayerState::Idle);
}

void Player::Reset()
{
	transform.position = definition_.startPosition;
	physics.velocity = {};

	hp_ = maxHp_;

	physics.isGrounded = true;
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
	return frame >= definition_.attackFirstFrame && frame <= definition_.attackLastFrame;
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
	const Vector2 attackExtent = definition_.attackExtent;

	if (facingRight_)
	{
		return
		{
			transform.position - Vector2{ 0.0f, attackExtent.y },
			transform.position + attackExtent
		};
	}

	return
	{
		transform.position - attackExtent,
		transform.position + Vector2{ 0.0f, attackExtent.y }
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
