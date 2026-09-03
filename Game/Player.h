#pragma once

#include "Entity/Character.h"

enum class PlayerState
{
	Idle,
	Run,
	JumpStart,
	JumpEnd,
	Attack,
	Dead
};

class Player : public Character
{
public:
	// 생성자
	Player();

	void Update(float deltaTime) override;

	PlayerState GetState() const;

	void TakeDamage(int damage, float attackerX) override;

	void StartAttack();
	void FinishAttack();

	void Reset();

	bool IsAttacking() const;

	bool IsInvincible() const;
	bool ShouldRender() const;

	AABB GetAttackHitBox() const;

private:
	void HandleInput();
	void ApplyGravity(float deltaTime);
	void UpdatePosition(float deltaTime);
	void ResolveGroundCollisions();
	void UpdateState();
	void UpdateDamageState(float deltaTime);

private:
	bool isGrounded_ = true;

	PlayerState state_ = PlayerState::Idle;

	bool attackHitRegistered_ = false;
	bool isInvincible_ = false;

	float invincibleTimer_ = 0.0f;
	float knockbackTimer_ = 0.0f;

	static constexpr float kInvincibleDuration = 2.4f;
	static constexpr float kKnockbackDuration = 0.4f;
	static constexpr float kKnockbackSpeed = 0.9f;

	static constexpr float kMoveSpeed = 0.8f;
	static constexpr float kJumpSpeed = 1.5f;
	static constexpr float kGravity = -3.0f;

	static constexpr float kGroundY = -0.3f;
};