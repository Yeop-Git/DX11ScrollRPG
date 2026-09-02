#pragma once

#include "Collision.h"

enum class PlayerState
{
	Idle,
	Run,
	JumpStart,
	JumpEnd,
	Attack,
	Dead
};

class Player
{
public:
	void Update(float deltaTime);

	float GetX() const;
	float GetY() const;

	bool IsFacingRight() const;
	PlayerState GetState() const;

	void TakeDamage(int damage, float attackerX);

	void StartAttack();
	void FinishAttack();
	void FinishHit();

	void Reset();

	bool IsAttacking() const;
	bool IsDead() const;

	bool IsInvincible() const;
	bool ShouldRender() const;

	AABB GetBodyBox() const;
	AABB GetAttackHitBox() const;

private:
	void HandleInput();
	void ApplyGravity(float deltaTime);
	void UpdatePosition(float deltaTime);
	void ResolveGroundCollisions();
	void UpdateState();
	void UpdateDamageState(float deltaTime);

private:
	// Transform
	float x_ = 0.0f;
	float y_ = -0.2f;

	// Velocity
	float velocityX_ = 0.0f;
	float velocityY_ = 0.0f;

	bool isGrounded_ = true;
	bool facingRight_ = true;

	PlayerState state_ = PlayerState::Idle;

	// HP
	int hp_ = 3;
	int maxHp_ = 3;

	bool isInvincible_ = false;

	float invincibleTimer_ = 0.0f;
	float knockbackTimer_ = 0.0f;

	static constexpr float kInvincibleDuration = 2.4f;
	static constexpr float kKnockbackDuration = 0.4f;
	static constexpr float kKnockbackSpeed = 0.9f;

	static constexpr float kMoveSpeed = 0.8f;
	static constexpr float kJumpSpeed = 1.5f;
	static constexpr float kGravity = -3.0f;

	static constexpr float kHalfWidth = 0.1f;
	static constexpr float kHalfHeight = 0.18f;

	static constexpr float kGroundY = -0.3f;
};