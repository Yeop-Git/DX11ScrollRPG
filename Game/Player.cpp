#include "Player.h"

#include <Windows.h>
#include <algorithm>
#include <cmath>

Player::Player(const GameData& data)
	: definition_(data.player), stat_(data.playerStat)
{
	// 검증된 공유 정의를 사용하고, 현재 HP와 Animator 재생 상태만 객체별로 가진다.
	maxHp_ = stat_.GetMaxHp();
	attacks_[0] = std::make_unique<MeleeAttack>(data.attacks[0]);
	attacks_[1] = std::make_unique<ProjectileAttack>(data.attacks[1], AttackSlot::Q);
	attacks_[2] = std::make_unique<ProjectileAttack>(data.attacks[2], AttackSlot::W);
	attacks_[3] = std::make_unique<GroundAreaAttack>(data.attacks[3]);
	attacks_[4] = std::make_unique<PeriodicAttack>(data.attacks[4]);
	hp_ = maxHp_;
	renderOffsetY = definition_.renderOffsetY;
	collider.halfSize = definition_.colliderHalfSize;
	transform.position = definition_.startPosition;
	animator_.Play(definition_.idle);
	physics.isGrounded = true;
}

Player::~Player() = default;

void Player::Update(float deltaTime)
{
	if (!IsDead()) for (auto& attack : attacks_) attack->UpdateCooldown(deltaTime);
	failureTimer_ = (std::max)(0.0f, failureTimer_ - deltaTime);
	const bool wasGrounded = physics.isGrounded;

	animator_.Update(deltaTime);

	UpdateDamageState(deltaTime);

	HandleInput();


	ClampWorld();

	if ((state_ == PlayerState::Attack || state_ == PlayerState::SkillCast))
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
	case PlayerState::SkillCast:
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

	// 공격은 World가 수집한 요청을 물리/지면 처리 뒤 한 번 소비한다.
	if (!GetForegroundWindow()) return;
	DWORD processId = 0;
	GetWindowThreadProcessId(GetForegroundWindow(), &processId);
	if (processId != GetCurrentProcessId()) return;

	if ((state_ == PlayerState::Attack || state_ == PlayerState::SkillCast)) return;

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
	if ((state_ == PlayerState::Attack || state_ == PlayerState::SkillCast)) return;
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
	case PlayerState::SkillCast:
		animator_.Play(definition_.attack);
		break;

	case PlayerState::Dead:
		animator_.Play(definition_.dead);
		break;
	}
}

DamageResult Player::TakeDamage(const DamageRequest& request)
{
	if (state_ == PlayerState::Dead || isInvincible_ || request.amount <= 0) return DamageResult::Ignored;

	hp_ -= request.amount;


	if (hp_ <= 0)
	{
		hp_ = 0;

		physics.velocity = {};

		ChangeState(PlayerState::Dead);
		return DamageResult::Killed;
	}

	// 무적 시작
	isInvincible_ = true;
	invincibleTimer_ = definition_.invincibleDuration;

	// 넉백 시작
	knockbackTimer_ = definition_.knockbackDuration;

	physics.velocity = definition_.knockbackSpeed;
	if (transform.position.x < request.attackerX) physics.velocity.x *= -1.0f;
	physics.isGrounded = false;
	return DamageResult::Applied;
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
	if ((state_ == PlayerState::Attack || state_ == PlayerState::SkillCast)) return;
	if (state_ == PlayerState::Dead) return;
	if (!physics.isGrounded) return;

	ChangeState(PlayerState::Attack);
	attackHitRegistered_ = false;
	physics.velocity.x = 0.0f;
}

void Player::FinishAttack()
{
	if ((state_ == PlayerState::Attack || state_ == PlayerState::SkillCast)) ChangeState(PlayerState::Idle);
}

void Player::Reset()
{
	stat_.Reset();
	maxHp_ = stat_.GetMaxHp();
	for (auto& attack : attacks_) attack->Reset();
	requestedAttack_ = failedSlot_ = -1;
	failureTimer_ = 0.0f;
	transform.position = definition_.startPosition;
	physics.velocity = {};

	hp_ = maxHp_;

	physics.isGrounded = true;
	facingRight_ = true;

	isInvincible_ = false;
	invincibleTimer_ = 0.0f;
	knockbackTimer_ = 0.0f;
	attackHitRegistered_ = false;

	state_ = PlayerState::Idle;
	animator_.Play(definition_.idle);
}

bool Player::IsAttackFrameActive() const
{
	if (state_ != PlayerState::Attack) return false;

	const int frame = animator_.GetCurrentFrame();
	return frame >= GetNormalAttack().firstActiveFrame && frame <= GetNormalAttack().lastActiveFrame;
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
	const Vector2 attackExtent = GetNormalAttack().hitBox;

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

void Player::StartSkillCast()
{
	ChangeState(PlayerState::SkillCast);
	physics.velocity.x = 0.0f;
}
void Player::GainExperience(uint64_t reward)
{
	stat_.AddExperience(reward);
	maxHp_ = stat_.GetMaxHp();
	hp_ = (std::min)(hp_, maxHp_);
}
AttackAvailability Player::GetAttackState() const
{
	if (IsDead()) return AttackAvailability::Dead;
	if (!combatEnabled_) return AttackAvailability::Disabled;
	if (knockbackTimer_ > 0.0f) return AttackAvailability::Hurt;
	if (!physics.isGrounded) return AttackAvailability::Airborne;
	if (state_ == PlayerState::Attack || state_ == PlayerState::SkillCast) return AttackAvailability::Casting;
	return AttackAvailability::Ready;
}
void Player::ConsumeAttack(GameWorld& world)
{
	const int slot = requestedAttack_; requestedAttack_ = -1;
	if (slot < 0) return;
	const auto result = attacks_.at(slot)->TryUse(*this, world);
	if (result == AttackAvailability::PoolFull || result == AttackAvailability::NoGround)
	{
		failedSlot_ = slot; lastFailure_ = result; failureTimer_ = 1.0f;
	}
}

std::array<AttackHudData, 5> Player::GetAttackHudData() const
{
	std::array<AttackHudData, 5> result;
	for (size_t i = 0; i < attacks_.size(); ++i)
	{
		const auto& attack = *attacks_[i];
		const auto& definition = attack.GetDefinition();
		auto state = attack.GetAvailability(*this);
		if (state == AttackAvailability::Ready && failureTimer_ > 0.0f && failedSlot_ == i) state = lastFailure_;
		result[i] = {definition.name, definition.inputKey, definition.requiredLevel, definition.cooldown, attack.GetRemainingCooldown(), state, definition.icon};
	}
	return result;
}
