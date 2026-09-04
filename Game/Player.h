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
	RenderInfo GetRenderInfo() const override;

	PlayerState GetState() const;

	void TakeDamage(int damage, float attackerX) override;

	void StartAttack();
	void FinishAttack();

	void Reset();

	bool IsAttackFrameActive() const;
	bool CanRegisterAttackHit() const;
	void RegisterAttackHit();

	bool IsInvincible() const;
	bool ShouldRender() const;

	AABB GetAttackHitBox() const;

	const Animator& GetAnimator() const;

private:
	void HandleInput();
	void ApplyGravity(float deltaTime);
	void UpdateState(bool wasGrounded);
	void ChangeState(PlayerState newState);
	void UpdateDamageState(float deltaTime);

private:
	PlayerState state_ = PlayerState::Idle;

	// Animation Clips
	AnimationClip idleClip_;
	AnimationClip runClip_;
	AnimationClip jumpStartClip_;
	AnimationClip jumpEndClip_;
	AnimationClip attackClip_;
	AnimationClip deadClip_;

	// Attack
	bool attackHitRegistered_ = false;

	// Hurt
	bool isInvincible_ = false;
	float invincibleTimer_ = 0.0f;
	float knockbackTimer_ = 0.0f;

	static constexpr float kInvincibleDuration = 2.4f;
	static constexpr float kKnockbackDuration = 0.4f;
	static constexpr Vector2 kKnockbackSpeed{ 1.8f, 1.0f };

	// Run
	static constexpr float kMoveSpeed = 0.8f;
	static constexpr float kJumpSpeed = 1.5f;
	static constexpr float kGravity = -3.0f;

	static constexpr Vector2 kStartPosition{ 0.0f, -0.12f };

	// Animation Clip
	static constexpr int kIdleClipCount = 4;
	static constexpr int kRunClipCount = 8;
	static constexpr int kJumpStartClipCount = 4;
	static constexpr int kJumpEndClipCount = 3;
	static constexpr int kAttackClipCount = 8;
	static constexpr int kDeadClipCount = 8;
};
