#pragma once

#include "Entity/Character.h"
#include "Data/GameData.h"

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
	explicit Player(const PlayerDefinition& definition);

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
	void UpdateState(bool wasGrounded);
	void ChangeState(PlayerState newState);
	void UpdateDamageState(float deltaTime);

private:
	PlayerState state_ = PlayerState::Idle;

	// Application의 불변 설정을 빌린다. 설정은 이 객체보다 오래 살아야 한다.
	const PlayerDefinition& definition_;

	// Attack
	bool attackHitRegistered_ = false;

	// Hurt
	bool isInvincible_ = false;
	float invincibleTimer_ = 0.0f;
	float knockbackTimer_ = 0.0f;

};
